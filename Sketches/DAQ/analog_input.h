/*
    fill s_array with analog input.
    s_nbr, s_rate and analogInPin are user defined (via serial input)
*/
void get_samples()
{
  // show AD start on LCD
  if (LCD) LCD_conversion_started();


  // start sampling, multiplex when more then one input.
  unsigned long time_target;                                  // time to wait before firing the next conversion burst  (in microsec)
  // calculate in nanosec to make it possible to show an accurate delta on the LCD
  long s_delay_nano = 1000000000 / s_rate;                    // delay between samples is sec/s_rate in nanoseconds
  // unfortunately is the best time measurement in micros(), so divide by 1000
  long s_delay_micro = (s_delay_nano / 1000) - sample_duration_correction; // correct the extra time between samples, in fact the sampling and loop-time
  int ii = 0;                                                 // index sample array
  int port_index = 0;                                         // pointer to the ports
  unsigned int port_num = 0b0000000000000000;                 // use bit 13-15 to indicate which analog port
  unsigned long temp_time = micros();                         // LCD -> calcultion of the time per sample
  do
  {
    time_target = micros() + s_delay_micro;                   // take the next shot at this time
    do
    {
      s_array[ii++] = analogRead(ports[port_index++]) + port_num;  // Save data: sample value stored in bit 0-11, portnumber in bit 12-14
      port_num =  port_num + 0b0001000000000000;              // calculate the next port number for the 4 most significant bits
    } while (port_index < analog_ports);
    while (micros() < time_target) {};                        // wait till it's time for the next burst, about 1.24 microsecond/cycle

    port_index = 0;
    port_num = 0b0000000000000000;                            // reset port number
  } while (ii < s_nbr);

  // if LCD anabled, show on LCD the actual sample rate
  if (LCD) LCD_show_actual(temp_time);
}
