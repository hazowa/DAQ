/* SINGLE Core buffered_multiple_analog_in_LCD.ino (the second core is for future usage ;-)
 *  From version V.020 a new LCD library (hd44780 library package, https://github.com/duinoWitchery/hd44780)
 *  The second core is for future use (circular buffer, signal level, programmable signal generator)

   General description buffered analog input
   Purpose:
    -read a burst of data from 1-8 of the analog input ports. Optional: wait for trigger.
    -collect the data in a buffer
    -when ready collecting data, send the data through the serial port to a connected console/device

   Communication parameters:
    -Serial port, baudrate: 230400

    ADC parameters:
    -[category] [number of ports] [number of samples] [samples per second] [trigger on/off] [trigger event] [debounce time]
     As an example, console input:  ADC 4 10000 2000 1 1 100
     results in:
        reading of the first 4 analog input port
        read 10.000 samples to buffer (40.000 samples in total = 80.000 bytes)
        read these samples in a rate of 2.000 per second per port
        Wait for trigger event
        trigger event 1 = wait for the signal on the trigger port changes from low to high.
        trigger when the event is 100 microseconds stable
     The AD conversion starts on the specified trigger event, after the parameter string is send

    TEXT parameters:
    -[category] [col] [row] [text]
     example: TEXT 0 1 blabla
     results in:
     set cursor to row-column coordinates and print the text 'blabla' 
     
    LEVEL parameters: (partly implemented)
    -[category] [col] [row]
     example: LEVEL 0 1
     results in:
     set cursor to row-col and show level indicators for all analog and trigger input 

     ATT parameters: (partly implemented)
     -[category] [channel][attenuation]         (adjust the attenuators for all or per channel)
      example: ATT 0 ADC_11db
      results in:
         ADC_11db: sets no attenuation (1V input = ADC reading of 1088)= 0,919mV resolution. Max voltage: 3,76V
         ADC_6db: sets an attenuation of 1.34 (1V input = ADC reading of 2086)= 0,479mV resolution. Max voltage: 1.962V
         ADC_2_5db: sets an attenuation of 1.5 (1V input = ADC reading of 2975)= 0,336mV resolution. Max voltage: 1.376V
         ADC_0db: sets an attenuation of 3.6 (1V input = ADC reading of 3959) = 0,256mV resolution. Max voltage: 1.049V

      DAC. Python composes a signal and sent it to the DAQ's circular buffer to output through the DAC. (yet not implemented, the second core will be used!)
      -[category] [port] [length] [array of values]
      example: DAC port_1 255 sinus
     
     Hardware:
      ESP32 DevkitV1

      Python example: buffered_analog_in_User_input_V0.5.py or later.
*/

#include "adc.h"                    // source: https://github.com/espressif/arduino-esp32/blob/master/tools/sdk/include/driver/driver/adc.h
#include <Wire.h>
#include <hd44780.h>                       // main hd44780 header, https://github.com/duinoWitchery/hd44780
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header


#define baudrate 230400             // related to quality of the USB cable, performance and chipset of the connected pc.  

bool debug = false;                 // get verbose console output for debugging when 'true'
bool LCD = true;                    // in case i2c LCD conected, show output

// define i2c pins and LCD address
#define SCL_pin 22                  // don't forget the pullup resistors for SCL and SDA to 3.3V
#define SDA_pin 21
#define LCD_address 0x3F

// AD-sample buffer parameters
const unsigned int s_nbr_max = 50000; // maximum number of samples, check dynamic memory space for local variables
uint16_t s_array[s_nbr_max];        // array to store samples
unsigned int s_rate_max = 100000;   // maximum number of samples per second
unsigned int sample_duration_correction = 0;  // sample function consumes some extra time, correction time in microsec.

// input buffer for user input 
const int input_buf_size = 30;      // max. expected characters from serial input
char buf[input_buf_size];           // serial input buffer

// these vars will be filled with parsed input data
char cat[5];                        // category.  After "ADC" the AD conversion parameters will follow
                                    //            After "ATT" the sensitivity of the inputports will follow etc. (See 'User_input.h)
int analog_ports = 1;               // number of analog ports, default = 1
int port_max = 8;                   // maximum number of analog ports
unsigned int s_rate;                // nr of samples/sec
unsigned int s_nbr;                 // number of samples in total
// crosstable GPIO port number -> analog input 0 - 7
const int ports[] = {36, 39, 34, 35, 32, 33, 25, 26}; // define portnumbers for the analog ports

//// trigger on/off and event
//int trigger_port = 27;              // port 27 on esp32 defined as trigger port
//int trigger = 0;                    // when '1' wait for event on port 27 to start conversion, '0' ignore
//#define Low 0                       // 0 = Low (trigger when pin is LOW)
//#define High 1                      // 1 = High (trigger when pin is HIGH)
//#define Change 2                    // 2 = Change ( trigger when the pin changes value from LOW to HIGH or HIGH to LOW)
//#define Rising 3                    // 3 = Rising (trigger when the pin goes from LOW to HIGH)
//#define Falling 4                   // 4 = Falling (trigger when the pin goes from HIGH to LOW)
//#define Ignore 99                   // 99 = Ignore trigger
//int trigger_event = Ignore;         // default trigger level/edge is low. See setup(), if pullup or pulldown resistor is active
//unsigned int debounce = 100;        // Time in microseconds a level must be stable to prevent jitter

// define on board LED port
const int LED = 2;                  // LED port, off during sampling, blinks during transfering serial data

// Used for category text, show user text on display.
int  TEXT_row;
int  TEXT_col;
char TEXT_text[20];



/*
 * NOTE:
 * - keep these files in the same map as the .ino sketch
 * - the include files must be defined before the function call in the main sketch
 * - inside the include files, plave the called function above the calling function
 */
#include"trigger.h"                 // All functions related to external trigger signal input
#include "LCD.h"                    // LCD settings and LCD related functions
#include "analog_input.h"           // All function relates toe AD conversion
#include "sample_output.h"          // All functions related to stream buffer to connected computer
#include "user_input.h"             // All functions related to user input

#include "cat_ADC.h"                // Do ADC conversion after parsing user input
#include "cat_LEVEL.h"              // Show levels 
#include "cat_TEXT.h"               // Show user text on display
#include "cat_ATT.h"                // adjust the attenuators for all or per channel
#include "cat_DAC.h"                // compose a signal and sent through the DAC


void setup()
{
  Serial.begin(baudrate);
/*        
 *  Attenuation:
 *  ADC_11db: sets no attenuation (1V input = ADC reading of 1088)= 0,919mV resolution. Max voltage: 3,76V
 *  ADC_6db: sets an attenuation of 1.34 (1V input = ADC reading of 2086)= 0,479mV resolution. Max voltage: 1.962V
 *  ADC_2_5db: sets an attenuation of 1.5 (1V input = ADC reading of 2975)= 0,336mV resolution. Max voltage: 1.376V
 *  ADC_0db: sets an attenuation of 3.6 (1V input = ADC reading of 3959) = 0,256mV resolution. Max voltage: 1.049V
*/
  analogSetAttenuation(ADC_11db);
  ///adc1_config_width(ADC_WIDTH_BIT_9);
  

  // initiate analog ports as input
  for (int s = 0; s < port_max; s++) pinMode(ports[analog_ports], INPUT);

  // initiate LED (indicates AD conversion)
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);          // LED off during sampling

  // initiate Trigger port connect these pins with built-in resistors to 3.3V
  pinMode(trigger_port, INPUT_PULLUP);

  // initiate the LCD module, create the special level meter characters and show some text to the world
  if (LCD)
  {
    LCD_initiate();
    delay(2000);                  // time to read the text on the LCD 
    lcd.clear();
  }
                                  
  if (LCD) LCD_waiting_for_input(0,0);  // show some text on the LCD display
}

void loop()
{
  // if input available start processing, else keep waiting for serial input
  if (serial_input())           // result in buf[]
  {
    fill_cat();                 // first parse the category from input
    // read selected analog inputs 
    if (strcmp(cat, "ADC") == 0)   cat_ADC();     // implemented
    // show evelop from analog input signals
    if (strcmp(cat, "LEVEL") == 0) cat_LEVEL();   // partly implemented 
    // show text on the optional display
    if (strcmp(cat, "TEXT") == 0)  cat_TEXT();    // implemented
    // set attenuation for aall or for selected analog input ports
    if (strcmp(cat, "ATT") == 0)   cat_ATT();     // partly implemented
    // compose a signal and sent through the DAC
    if (strcmp(cat, "DAC") == 0)   cat_DAC();     // not yet implemented
    
    if (LCD) LCD_level_redraw = true;             // temporary: rebuild legend LCD level indicator
  }
  else 
  {
    // show input level analog ports and trigger, as long there's no user input. 
    if (LCD)
    { 
      LCD_level(11, 0);       // show levels on display starting: col, row 
    }
  }
}
