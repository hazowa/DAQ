


#define maxsamp 1000  // maximum array size  
#define dacmax 255    // maximum DAC input value (8 bits) 
#define DAC 25       // DAC GPIO port number. DAC1 = 25, DAC2 = 26

int nsamp = 256;      // user-configurable, default is 256 samples per waveform. Sinus, about 520Hz
byte wave[maxsamp];   // create waveform storage
const float pi=3.14159265;


/*
 * Fill a byte array with size 'nsamp' with values as big as 'dacmax'
 * In this case with a sine wave
 */
// create waveform in array
void wave_sinus(void)
{
  for (int isamp=0; isamp < nsamp; ++isamp)
  {
    float phip=(isamp+0.5)/nsamp;
    float phi=2*pi*phip;
    //Sine
    wave[isamp] = (sin(phi)+1.0)*dacmax/2;   
  }
}

void wave_sinus(void) ;                // prototype is required when function is outside this TAB?



/*
 * Build waveform in array wave_xxxx[] and send the values to th DAC on bord DAC1
 * There is some interference when writing to i2c LCD UITZOEKEN HOE DIT KAN.....
 */
void cat_DAC_loop( void * parameter )
{
  unsigned int i = 0;

  // fill the array with the desired waveform
  wave_sinus();
  
  /*   
   *  As long as the next call to esp_task_wdt_reset() occurs before the watchdog timer expires, all is well
   *  In the infinite loop there is no call to a function that resets the timer. 
   *  A straight forward solution is to disable the watchdag timer....
   */
  disableCore0WDT();                    // disable the watchdog timer

  /* if timing is necessary, use delayMicroseconds(delay)
   delay = no: 704Hz  -> ca. 5.2 microsec per sample
            1: 549Hz 
            2: 480Hz
            3: 427Hz
            4: 386Hz
            ..
           10: 242Hz
  */
  // send array to DAC, infinite loop.
  for (;;)
  {
    while (!DAC_run) vTaskDelay(10);      // wait till DAC_run is true
    
//    sigmaDeltaWrite(0, wave[i++]);     //pwm is as fast as dacWrite
    dacWrite(DAC, wave[i]);              //Write data to DAC
//    delayMicroseconds(10);
    if (i >= nsamp-1) i = 0; else i++;    // pointer to array, start all over when 'nsamp' samples are send to DAC
  }
}


void cat_DAC()
{
  DAC_parse();                // fill all variabel with user input
  Serial.println(DAC_startstop);
  if (strcmp(DAC_startstop, "START") == 0) DAC_run = true; else DAC_run = false;
  Serial.println(DAC_run);
  
}
