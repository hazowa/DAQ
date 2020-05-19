// define LCD
hd44780_I2Cexp lcd; // declare lcd object: auto locate & auto config expander chip
// LCD geometry
const int LCD_COLS = 20;
const int LCD_ROWS = 4;



//LiquidCrystal_I2C lcd(LCD_address, totalColumns, totalRows);
bool LCD_level_redraw = true;     // Only once rebuild level indicator 

int LCD_level_max[9];             // save the previous maximum value of the level measurement per port
int LCD_level_min[9];             // save the previous minimum value of the level measurement per port
int prev_trigger;                 // remember the previous trigger level

unsigned long LCD_level_decay_time = 1000;// (ms) decrease the level every 'LCD_level_decay_time' with 'LCD_level_decay'
unsigned long LCD_level_time;     // remember when the last substration took place
unsigned long LCD_level_decay = 1;// the number to decrease every 'LCD_level_decay_time'

bool LCD_show_level_indicator = true;   // when true, show input levels analog and trigger ports

/* ---------------------------------------------------------------------------------------------
 * To show the maximum AND the minimum value of a signal on an LCD 20x4 display 
 * the 8 programmable charachters must be redefined. Read the excel sheet "LCD-CrossTabel.xlsx"
 * for an explanation.
*/
const int Level_min = 16;
const int Level_max = 16;
const int Segments = 2;
int Segment_high = 1;
int Segment_low  = 0;

uint8_t ElementLowHigh[Level_min][Level_max][Segments] =
{
  {{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32}},
  {{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32},{ 1,32}},
  {{95,32},{95,32},{95,32},{95,32},{95,32},{95,32},{95,32},{95,32},{95,32},{95,32},{95,32},{95,32},{95,32},{95,32},{95,32},{95,32}},
  {{95,32},{95,32},{95,32},{95,32},{95,32},{95,32},{95,32},{95,32},{95,32},{95,32},{95,32},{95,32},{95,32},{95,32},{95,32},{95,32}},
  {{45,32},{ 8,32},{ 6,32},{61,32},{45,32},{45,32},{45,32},{45,32},{45,32},{45,32},{45,32},{45,32},{45,32},{45,32},{45,32},{45,32}},
  {{45,32},{ 8,32},{ 6,32},{61,32},{61,32},{45,32},{45,32},{45,32},{45,32},{45,32},{45,32},{45,32},{45,32},{45,32},{45,32},{45,32}},
  {{ 2,32},{ 8,32},{ 4,32},{ 4,32},{61,32},{ 5,32},{ 2,32},{ 2,32},{ 2,32},{ 2,32},{ 2,32},{ 2,32},{ 2,32},{ 2,32},{ 2,32},{ 2,32}},
  {{ 3,32},{ 7,32},{ 4,32},{ 4,32},{ 5,32},{ 5,32},{ 3,32},{ 3,32},{ 3,32},{ 3,32},{ 3,32},{ 3,32},{ 3,32},{ 3,32},{ 3,32},{ 3,32}},
  {{32, 1},{ 1, 1},{95, 1},{95, 1},{45, 1},{45, 1},{ 3, 1},{ 3, 1},{32, 1},{32, 1},{32, 1},{32, 1},{32, 1},{32, 1},{32, 1},{32, 1}},
  {{32, 1},{ 1, 1},{95, 1},{95, 1},{45, 1},{45, 1},{ 3, 1},{ 3, 1},{32, 1},{32, 1},{32, 1},{32, 1},{32, 1},{32, 1},{32, 1},{32, 1}},
  {{32,95},{ 1,95},{95,95},{95,95},{45,95},{45,95},{ 3,95},{ 3,95},{32,95},{32, 1},{32,95},{32,95},{32,95},{32,95},{32,95},{32,95}},
  {{32,95},{ 1,95},{95,95},{95,95},{45,95},{45,95},{ 3,95},{ 3,95},{32,95},{32, 6},{32,95},{32,95},{32,95},{32,95},{32,95},{95,32}},
  {{32,45},{ 1,45},{95,45},{95,45},{45,45},{45,45},{ 3,45},{ 3,45},{32,45},{32, 6},{32, 6},{32, 6},{32,45},{32,45},{32,45},{32,45}},
  {{32,45},{ 1,45},{95,45},{95,45},{45,45},{45,45},{ 3,45},{ 3,45},{32,45},{32, 4},{32, 6},{32, 6},{32,61},{32,45},{32,45},{32,45}},
  {{32, 2},{ 1, 2},{95, 2},{95, 2},{45, 2},{45, 2},{ 3, 2},{ 3, 2},{32, 2},{32, 4},{32, 4},{32, 4},{32,61},{32, 2},{32, 2},{32, 2}},
  {{32, 3},{ 1, 3},{95, 3},{95, 3},{45, 3},{45, 3},{ 3, 3},{ 3, 3},{32, 3},{32, 7},{32, 7},{32, 4},{32, 5},{32, 3},{32, 3},{32, 3}}
};


uint8_t ch1[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B11111};
uint8_t ch2[8] = {B00000, B11111, B00000, B00000, B00000, B00000, B00000, B00000};
uint8_t ch3[8] = {B11111, B00000, B00000, B00000, B00000, B00000, B00000, B00000};
uint8_t ch4[8] = {B00000, B11111, B00000, B00000, B00000, B00000, B11111, B00000};
uint8_t ch5[8] = {B00000, B11111, B00000, B00000, B11111, B00000, B00000, B00000};
uint8_t ch6[8] = {B00000, B00000, B00000, B11111, B00000, B00000, B11111, B00000};
uint8_t ch7[8] = {B11111, B00000, B00000, B00000, B00000, B00000, B00000, B11111};
uint8_t ch8[8] = {B00000, B00000, B00000, B11111, B00000, B00000, B00000, B11111};
// ---------------------------------------------------------------------------------------------

/*
 * initiate the LCD module, create the special level meter characters and say something to the world
 */
void LCD_initiate()
{
  // initiate LCD
  int status = lcd.begin(LCD_COLS, LCD_ROWS);
  if(status) // non zero status means it was unsuccesful
  {
    // hd44780 has a fatalError() routine that blinks an led if possible
    // begin() failed so blink error code using the onboard LED if possible
    hd44780::fatalError(status); // does not return
  }

  // Load LCD panel with new characters (max 8) 
  lcd.createChar(1, ch1);
  lcd.createChar(2, ch2);
  lcd.createChar(3, ch3);
  lcd.createChar(4, ch4);
  lcd.createChar(5, ch5);
  lcd.createChar(6, ch6);
  lcd.createChar(7, ch7);
  lcd.createChar(8, ch8);

  
  lcd.backlight(); // use to turn on and turn off LCD back light
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Data Acquisition");
  
  lcd.setCursor(3, 1);
  lcd.print("Arduino/Python");

  lcd.setCursor(1, 2);
  lcd.print("8xADC + 1xTrigger");
  
  lcd.setCursor(0, 3);
  lcd.print("Baudrate: "); lcd.print(baudrate);
}

/*
 * child of LCD_level, calculate the levels to characters and show the levels on LCD
 */
void LCD_show_level(int port, int level, int col, int row)
{ 
  int temp_max = LCD_level_max[port];             // save te previous value to prevent unnecessary write to LCD
  int temp_min = LCD_level_min[port];
  
  // show level for this port on column 'col'
  col = col + port;

  // port 8 is the trigger port
  if ((port == 8) && (prev_trigger != level))
  {
    // update display when changed
    lcd.setCursor(col, row);
    lcd.write(char(ElementLowHigh[level][level][Segment_high]));  
    
    lcd.setCursor(col, row+1);
    lcd.write(char(ElementLowHigh[level][level][Segment_low]));      
    prev_trigger = level;
  } 
  else //port 0-7 is the analoge input
  {
    // map the input value to 16 levels and calculate the envelope 
    // (minimum and maximum value of the converted input signal)
    level = map(level, 0, 4095 , 0 , 15);
    LCD_level_max[port] = max(level, LCD_level_max[port]);
    LCD_level_min[port] = min(level, LCD_level_min[port]);  


  // After 'LCD_level_decay_time' substract 'LCD_level_decay' from 'max' and add 'LCD_level_decay' to 'min'
  // The effect is that when the input value decreases the max-value will follow. When the AC component decreases
  // the difference between 'min' and 'max' will decrease.
    if (millis() > LCD_level_time)
    {
      LCD_level_time = millis() + LCD_level_decay_time;
      for (int i = 0; i <8; i++)
      {
        // after decay-time the minimum level is at most the same value as the maximum level
        if (LCD_level_min[i] < LCD_level_max[i])  LCD_level_min[i] = LCD_level_min[i] + LCD_level_decay;
        // after decay-time only subtract when maximum value > 0
        if (LCD_level_max[i] > 0) LCD_level_max[i] = LCD_level_max[i] - LCD_level_decay;
      }
    }
  
    // use the table 'ElementLowHigh' as a reference to turn on the right characters for the upper and lower segment.
    // show only when changed
    if (temp_max != LCD_level_max[port])
    {
      lcd.setCursor(col, row);
      lcd.write(char(ElementLowHigh[LCD_level_max[port]][LCD_level_min[port]][Segment_high])); 
    }
 
    if (temp_min != LCD_level_min[port])
    {
      lcd.setCursor(col, row+1);
      lcd.write(char(ElementLowHigh[LCD_level_max[port]][LCD_level_min[port]][Segment_low]));  
    }
  }
}



/*
 * in 'LCD_initiate' we made some special characters for the input level indicator.  
 * show the levels from the analoge inputs and trigger input.
 */
void LCD_level(int col, int row)
{
  unsigned int level;                             // store the input value
  int port_index = 0;                             // pointer to the input port

  if (LCD_level_redraw)                           // show ones legend and text
  {
    lcd.setCursor(col, row++);
    lcd.print("--LEVEL--");
               
    lcd.setCursor(col, row++);
    lcd.print("12345678T");   
    LCD_level_redraw = false;                     //rebuild LCD level indicator only ones
  }
  else row = row + 2;

  /* 
   * read analog port and send the result to LCD_show_level() to show the combined maximum and minimal value on the LCD-display
   * do this for all analog ports
 */
  do
  {
    level = analogRead(ports[port_index]);        // store data
    LCD_show_level(port_index++, level, col, row);// show input level on display
  } while (port_index < port_max);

  // the same for the trigger port
  if (digitalRead(trigger_port))                  // read the DIGITAL port, assign 15 when true or zero when false
    LCD_show_level(port_index, 15, col, row); 
  else 
    LCD_show_level(port_index, 0, col, row); 
}

void LCD_waiting_for_input(int col, int row)
{
      lcd.setCursor(col, row++);
      lcd.print("Waiting");
      lcd.setCursor(col, row++);
      lcd.print("for");
      lcd.setCursor(col, row++);
      lcd.print("Input...");  
}


/*
 * measure the time it takes to read the defined number of samples and calculate the real sample rate
 * show the results on LCD screen
 */
void LCD_show_actual(unsigned long temp_time)
{
  long actual_total_sample_time_micro = micros() - temp_time;          // actual time to read all the samples
  float actual_time_per_sample_micro = float(actual_total_sample_time_micro) / float(s_nbr);
  long actual_sample_freq_micro = (1000000/actual_time_per_sample_micro) / analog_ports;  
  
  lcd.setCursor(0, 3);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print("Real:");
  lcd.print(actual_sample_freq_micro);
}


/*
 * show the translated parameter on LCD
 */
void LCD_show_parsed_input()
{
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Prts:");
  lcd.print(analog_ports);

  lcd.setCursor(0, 1);
  lcd.print("Smpl:");
  lcd.print(s_nbr/analog_ports);

  lcd.setCursor(0, 2);
  lcd.print("S/se:");
  lcd.print(s_rate);
}


/*
 * Show on LCD that the conversion has started
 */
void LCD_conversion_started()
{
  lcd.setCursor(0, 3);
  lcd.print("ADC started ...");
}
