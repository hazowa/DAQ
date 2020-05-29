/*  ESP32 based data acquisition device 
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

    A few other functionalities and wishes, these change from day to day, until the specifications are clear
    
    TEXT parameters:
    -[category] [col] [row] [text]
     example: TEXT 0 1 blabla
     results in:
     set cursor to row-column coordinates and print the text 'blabla' 
     !Spaces are not supported for the time being
     
    LEVEL parameters: (partly implemented)
    -[category] [col] [row]
     example: LEVEL 0 1
     results in:
     set cursor to row-col and show level indicators for all analog and trigger input
     !So far, the level is always shown when there are no other activities

     ATT parameters: (partly implemented)
     -[category] [channel][attenuation]         (adjust the attenuators for all or per channel)
      example: ATT 0 ADC_11db
      results in:
         ADC_11db: sets no attenuation (1V input = ADC reading of 1088)= 0,919mV resolution. Max voltage: 3,76V
         ADC_6db: sets an attenuation of 1.34 (1V input = ADC reading of 2086)= 0,479mV resolution. Max voltage: 1.962V
         ADC_2_5db: sets an attenuation of 1.5 (1V input = ADC reading of 2975)= 0,336mV resolution. Max voltage: 1.376V
         ADC_0db: sets an attenuation of 3.6 (1V input = ADC reading of 3959) = 0,256mV resolution. Max voltage: 1.049V
      !The attenuation is set in setup()

      DAC. Python composes a signal and sent it to the DAQ's circular buffer to output through the DAC. 
      -[category] [port] [length] [array of values]
      example: DAC port_1 255 sinus
      !yet not implemented, it would be nice to use the second cpu for this

    Specific sensors
      DHT22: humidity and temperature module/sensor (one or more)
      -[category] [pinnr] [Humidity|Temperature]
      example: DHT22 12 Humidity
      Return:  Humidity reading from DHT22 module connected to pin 12 in percent (int).
      example: DHT22 12 Temperature
      Return:  Temperature reading from DHT22 module connected to pin 12 in degrees Celcius (float).
      !yet not implemented    

      BMP280: pressure an temperature sensor
      -[category] [i2c address] [Prssure|Temperature] [Resolution] 
      etc.
      
     Hardware:
      ESP32 DevkitV1

      Python example: buffered_analog_in_User_input_V0.5.py or later.
*/

#include "adc.h"                    // source: https://github.com/espressif/arduino-esp32/blob/master/tools/sdk/include/driver/driver/adc.h
#include <Wire.h>
#include <hd44780.h>                // main hd44780 header, https://github.com/duinoWitchery/hd44780
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header


#define baudrate 230400             // related to the performance and chipset/cable of the connected pc.  

bool debug = false;                 // get verbose console output for debugging when 'true'
bool LCD = true;                    // in case i2c LCD conected, show output

// define i2c pins and LCD address
#define SCL_pin 22                  // don't forget the pullup resistors for SCL and SDA to 3.3V, see electrical diagram
#define LCD_address 0x3F            // !display default address 0x2F. Check it with i2c_scanner

// AD-sample buffer parameters
const unsigned int s_nbr_max = 50000; // maximum number of samples, check dynamic memory space for local variables
uint16_t s_array[s_nbr_max];        // array to store samples
unsigned int s_rate_max = 100000;   // maximum number of samples per second
unsigned int sample_duration_correction = 0;  // sample function consumes some extra time, correction time in microsec.

// input buffer for user input 
const int input_buf_size = 30;      // max. expected characters from serial input
char buf[input_buf_size];           // serial input buffer

// these vars will be filled after parsing the user input data
char cat[5];                        // category.  (ADC, TEXT, ATT, DAC .....)
int analog_ports = 1;               // number of analog ports, default = 1
int port_max = 8;                   // maximum number of analog ports
unsigned int s_rate;                // nr of samples/sec
unsigned int s_nbr;                 // number of samples in total

const int ports[] = {36, 39, 34, 35, 32, 33, 25, 26}; // define portnumbers for the analog ports
                                                      // crosstable GPIO port number -> analog input 0 - 7
// define on board LED port
const int LED = 2;                  // LED port, off during sampling, blinks during transfering serial data

// Used for category text, show user text on display.
int  TEXT_row;
int  TEXT_col;
char TEXT_text[20];



/*
 * NOTE:
 * - keep these include files in the same map as the .ino sketch
 * - the include files must be defined before the function call in the main sketch
 * - inside the include files, plave the called function above the calling function
 */
#include "trigger.h"                 // All functions related to external trigger signal input
#include "LCD.h"                    // LCD settings and LCD related functions
#include "analog_input.h"           // All function relates toe AD conversion
#include "sample_output.h"          // All functions related to stream buffer to connected computer
#include "user_input.h"             // All functions related to user input

#include "cat_ADC.h"                // ADC conversion after parsing user input
#include "cat_LEVEL.h"              // Show levels 
#include "cat_TEXT.h"               // Show user text on display after parsing user input
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

  // initiate all analog ports as input
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
                                  
  if (LCD) LCD_waiting_for_input(0,0);  // show some text on the LCD display to indicate the status
}

void loop()
{
  // if input available start processing, in the meantime, show the input levels
  if (serial_input())           // if user input, put it in buf[]
  {
    fill_cat();                 // first parse the category from input (ADC, TEXT, LEVEL....)
    // if ADC, read selected analog inputs 
    if (strcmp(cat, "ADC") == 0)   cat_ADC();     // implemented
    // if LEVEL show evelop from analog input signals
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
