/*
 * Build waveform in array wave[] and send the values to th DAC on bord DAC1
 * There is some interference when writing to LCD UITZOEKEN HOE DIT KAN.....
 */


#define nsamp  1000      // 180 samples per waveform (sinus) gives 1KHz
#define dacmax 256
#define DAC1 25

byte wave[nsamp];
const float pi=3.14159265;

void cat_DAC_loop( void * parameter )
{
//  static uint8_t i = 0;
unsigned int i = 0;

/* 
 *  disable the watchdog timer 
 *  As long as the next call to esp_task_wdt_reset() occurs before the watchdog timer expires, all is well
 *  In the infinite loop there is no call to a function that resets the timer. 
 *  A straight forward solution is to disable the watchdag timer....
 */
disableCore0WDT();

// create waveform in array
  for (int isamp=0; isamp < nsamp; ++isamp)
  {
    float phip=(isamp+0.5)/nsamp;
    float phi=2*pi*phip;
    //Sine
    wave[isamp] = (sin(phi)+1.0)*dacmax/2;   
  }

// send array to DAC, infinite loop.
  for (;;)
  {
    // sigmaDeltaWrite(0, wave[i++]);       //pwm
    dacWrite(DAC1, wave[i]);              //Smooth waveform
    if (i >= nsamp-1) i = 0; else i++;

    /*
     delay = no: 704Hz  -> ca. 5.2 microsec per sample
              1: 549Hz 
              2: 480Hz
              3: 427Hz
              4: 386Hz
              ..
             10: 242Hz
    */
  }
}
