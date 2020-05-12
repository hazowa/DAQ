// define LCD
int totalColumns = 20;
int totalRows = 4;
LiquidCrystal_I2C lcd(0x3F, totalColumns, totalRows);
bool LCD_level_redraw = true;           // Only once rebuild level indicator 



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
  lcd.print("Real :");
  lcd.print(actual_sample_freq_micro);
}


/*
 * show the translated parameter on LCD
 */
void LCD_show_parsed_input()
{
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Ports:");
  lcd.print(analog_ports);

  lcd.setCursor(0, 1);
  lcd.print("Smpls:");
  lcd.print(s_nbr/analog_ports);

  lcd.setCursor(0, 2);
  lcd.print("S/sec:");
  lcd.print(s_rate);
}


bool LCD_show_level_indicator = true;      // when true, show input levels analog and trigger ports

// LCD Meter character definition
//byte ch1[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B11111};
//byte ch2[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B11111, B00000};
//byte ch3[8] = {B00000, B00000, B00000, B00000, B00000, B11111, B00000, B00000};
//byte ch4[8] = {B00000, B00000, B00000, B00000, B11111, B00000, B00000, B00000};
//byte ch5[8] = {B00000, B00000, B00000, B11111, B00000, B00000, B00000, B00000};
//byte ch6[8] = {B00000, B00000, B11111, B00000, B00000, B00000, B00000, B00000};
//byte ch7[8] = {B00000, B11111, B00000, B00000, B00000, B00000, B00000, B00000};
//byte ch8[8] = {B11111, B00000, B00000, B00000, B00000, B00000, B00000, B00000};

byte ch1[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B11111};
byte ch2[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B11111, B11111};
byte ch3[8] = {B00000, B00000, B00000, B00000, B00000, B11111, B11111, B00000};
byte ch4[8] = {B00000, B00000, B00000, B00000, B11111, B11111, B00000, B00000};
byte ch5[8] = {B00000, B00000, B00000, B11111, B11111, B00000, B00000, B00000};
byte ch6[8] = {B00000, B00000, B11111, B11111, B00000, B00000, B00000, B00000};
byte ch7[8] = {B00000, B11111, B11111, B00000, B00000, B00000, B00000, B00000};
byte ch8[8] = {B11111, B11111, B00000, B00000, B00000, B00000, B00000, B00000};




/*
 * Show on LCD that the conversion has started
 */
void LCD_conversion_started()
{
  lcd.setCursor(0, 3);
  lcd.print("ADC started ...");
}


/*
 * For testing the LCD level meter, call this function
 */
void LCD_meter_test(int strt_column, int strt_row)
{
  for (int r = 1; r <= 8; r++)
  {
    for (int c = 0; c < 9; c++)
    {
      lcd.setCursor(strt_column + c, strt_row);
      lcd.write(char(r));
      delay(5);
    }
  }
}


/*
 * initiate the LCD module, create the special level meter characters and say 'hello' to the world
 */
void LCD_initiate()
{
  // initiate LCD
  lcd.init();

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

  lcd.setCursor(0, 2);
  lcd.print("8xADC + 1xTriggerprt");
  
  lcd.setCursor(0, 3);
  lcd.print("Baudrate: "); lcd.print(baudrate);
}


/*
 * child of LCD_level, calculate the levels to characters and show the levels on LCD
 */
void LCD_show_level(int port, unsigned int level, int col, int row)
{
  level = map(level, 0, 4095 , 1 , 8);
  lcd.setCursor(col + port , row);
  lcd.write(char(level));
  //delayMicroseconds(10000);
}


/*
 * Threath
 * in 'LCD_initiate' we made some special characters for the input level indicator. 
 * As long as in the main task the variable LCD_show_level_indicator isn't reset , 
 * show the levels from the analoge inputs and from the trigger input.
 */
void LCD_level(int col, int row)
{
  unsigned int level;                             // store the input value
  int port_index = 0;                             // pointer to the input port

  if (LCD_level_redraw)
  {
    lcd.setCursor(col, row++);
    lcd.print("--LEVEL--");
               
    lcd.setCursor(col, row++);
    lcd.print("12345678T");   
    LCD_level_redraw = false;                       //rebuild LCD level indicator only ones
  }
  else row = row + 2;
  

  do
  {
    level = analogRead(ports[port_index]);        // store data
    LCD_show_level(port_index++, level, col, row);          // show input level on display
  } while (port_index < 8);
  
  if (digitalRead(trigger_port))                  // read the DIGITAL port, assign 4095 when true or zero when false
    LCD_show_level(port_index, 4095, col, row); 
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
      lcd.print("Input ..");  

      
}
