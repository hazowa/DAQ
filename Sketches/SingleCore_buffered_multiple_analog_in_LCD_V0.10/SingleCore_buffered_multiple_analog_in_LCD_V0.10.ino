/* SINGLE Core buffered_multiple_analog_in_LCD.ino

   General description buffered analog input
   Purpose:
    -read a burst of data from 1-8 of the analog input ports
    -collect the data in a buffer
    -when ready collecting data, send the data through the serial port to a connected console/device

   Communication parameters:
    -Serial port, 19200 baudrate (value depends on time-out because Arduino transmits data faster to pc buffer than Python can receive)
    -[number of ports] [space] [number of samples] [space] [samples per second] [trigger on/off] [trigger event] [debounce time] [Pre-trigger number of samples]
    -As an example, console input:  4 10000 20000 1 2 100 and Enter
     results in:
        4 analog input port
        the total number of samples to buffer is 10000 (250 per analog input)
        read these samples in a rate of 20000 per second
        Wait for trigger event
        trigger event 2 = Change
        trigger when the event is 100 microseconds stable
     The AD conversion starts immediately after pressing the Enter key

     Hardware:
      ESP32 DevkitV1

      Python example: buffered_analog_in_User_input_V0.5.py
*/

#include <LiquidCrystal_I2C.h>  //Frank de Brabander
#include "adc.h"                // source: https://github.com/espressif/arduino-esp32/blob/master/tools/sdk/include/driver/driver/adc.h

#define baudrate 230400              // 

bool debug = false;                 // get verbose console output for debugging when 'true'
bool LCD = true;                    // in case i2c LCD conected, show output
bool check_sample_time = false;     // gives info about sample timing on console


// define i2c pins
#define SCL_pin 22
#define SDA_pin 21

// AD-sample buffer parameters
const unsigned int s_nbr_max  = 50000;// maximum number of samples, check dynamic memory space for local vriables
uint16_t s_array[s_nbr_max];        // array to store samples
unsigned int s_rate_max = 100000;    // maximum number of samples per second
unsigned int sample_duration_correction = 0;   // time sample function consumes extra

// input buffer parameters
const int input_buf_size = 30;      // max. expected characters from serial input
char buf[input_buf_size];           // serial input buffer

// these vars will be filled with parsed input data
int analog_ports = 1;               // number of analog ports, default = 1
int port_max = 8;                   // maximum number of analog ports
int ports[] = {36, 39, 34, 35, 32, 33, 25, 26}; // define portnumbers for the analog ports
//int ports[] = {36, 39, 34, 35, 32, 33, 25, 26}; // define portnumbers for the analog ports
unsigned int s_rate;                // nr of samples/sec
unsigned int s_nbr;                 // number of samples in total

// trigger on/off and event
int trigger_port = 27;              // port 27 on esp32 defined as trigger port
int trigger = 0;                    // when '1' wait for event on port 27 to start conversion
#define Low 0                       // 0 = Low (trigger whenever pin is LOW)
#define High 1                      // 1 = High (trigger whenever pin is HIGH)
#define Change 2                    // 2 = Change ( trigger whenever the pin changes value from LOW to HIGH or HIGH to LOW)
#define Rising 3                    // 3 = Rising (trigger when the pin goes from LOW to HIGH)
#define Falling 4                   // 4 = Falling (trigger when the pin goes from HIGH to LOW)
#define Ignore 99                   // 99 = Ignore trigger
int trigger_event = Ignore;            // default trigger level/edge is low. See setup(), pulldown resistor is active
unsigned int debounce = 100;        // Time in microseconds a level must be stable to prevent jitter

// define LCD: see LCD.h

// define on board LED port
const int LED = 2;                  // LED port, off during sampling, blinks during transfering serial data
// LED blink rate
unsigned long timestamp;
unsigned long blink_period = 200;   // when transfering sample data LED blinks with this interval in ms

/*
 * keep these files in the same map as the .ino sketch
 */
#include "LCD.h"
#include "analog_input.h"           // de include files moeten vóór de aanroep zijn gedefinieerd
#include "sample_output.h"          // binnen de include files de aan te roepen functies boven de aanroep
#include "user_input.h"
#include"trigger.h"




void setup()
{
  Serial.begin(baudrate);
  //  Serial.print("setup() running on core ");
  //  Serial.println(xPortGetCoreID());

/*        
 *  Attenuation:
 *  ADC_0db: sets no attenuation (1V input = ADC reading of 1088)= 0,919mV resolution. Max voltage: 3,76V
 *  ADC_2_5db: sets an attenuation of 1.34 (1V input = ADC reading of 2086)= 0,479mV resolution. Max voltage: 1.962V
 *  ADC_6db: sets an attenuation of 1.5 (1V input = ADC reading of 2975)= 0,336mV resolution. Max voltage: 1.376V
 *  ADC_11db: sets an attenuation of 3.6 (1V input = ADC reading of 3959) = 0,256mV resolution. Max voltage: 1.049V
*/
  analogSetAttenuation(ADC_11db);

  // initiate analog ports as input
  for (int s = 0; s < port_max; s++) pinMode(ports[analog_ports], INPUT);

  // initiate LED (indicates AD conversion)
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);    // LED off during sampling

  // initiate Trigger port connect these pins with built-in resistors to 3.3V
  pinMode(trigger_port, INPUT_PULLUP);

  // initiate the LCD module, create the special level meter characters and say 'hello' to the world
  if (LCD)
  {
    LCD_initiate();
    delay(2000);
    lcd.clear();
  }

  if (debug)
  {
    Serial.println();
    Serial.println(" ---------------------------------------------------------------------------------------------------");
    Serial.println("|Buffered ADC parameters:                                                                           |");
    Serial.println("|Number of analog inputs, number of samples, sample rate, trigger (OFF=0, ON=1), trigger event (0-3)|");
    Serial.println("|All separated by spaces.                                                                           |");
    Serial.println("|(To prevent this message, set the boolean variable 'debug' to 'false')                             |");
    Serial.println(" ---------------------------------------------------------------------------------------------------");
  }
  if (LCD) LCD_waiting_for_input(0,0);
}

void loop()
{
  // if input available start processing, else keep waiting for serial input
  if (serial_input())       // result in buf[]
  {
    parse_input();          // results from buf[] to analogInPin, s_rate and s_nbr;
    check_parsed_input();   // prevent overflow
    show_parsed_input();    // in case debug is true, show input values on console
    if (LCD) LCD_show_parsed_input();// in case LCD is true, show input values on LCD display

    if (trigger == 1) TriggerEvent(trigger_port, trigger_event, debounce);

    digitalWrite(LED, LOW); // to indicate sampling has started, dim LED
    get_samples();          // get analog samples, parameters in
    digitalWrite(LED, HIGH);// to indicate sampling has stopped, turn on LED

    dump_sample_array();    // return samples to Python serial or console
    if (LCD) LCD_level_redraw = true;  //rebuild LCD level indicator
  }
  else 
  {
    if (LCD)
    { 
      LCD_level(11, 0);       // show input levelanalog portsand trigger, strats at parameter: col, row 

    }
  }
}
