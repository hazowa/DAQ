/*
     For debug purpose when using console for I/O. Set debug to 'true' to get verbose info
*/
void show_parsed_input()
{
  if (debug)
  {
    Serial.println();
    Serial.print("Category:           "); Serial.println(cat);
    Serial.print("Raw input:          "); Serial.println(buf);
    Serial.print("Number of AD ports  "); Serial.println(analog_ports);
    Serial.print("Number of samples:  "); Serial.println(s_nbr);
    Serial.print("Samples per second: "); Serial.println(s_rate);
    Serial.print("Trigger on port:    "); Serial.println(trigger_port);
    Serial.print("Trigger on/off:     "); Serial.println(trigger);
    Serial.print("Trigger event:      "); Serial.println(trigger_event);
    Serial.print("Hysteresis:         "); Serial.println(debounce);
  }
}


/*
    Fill serial read buffer with parameters until new line.
*/
int readline(int readch, char *buffer, int len)
{
  static int pos = 0;
  int rpos;

  if (readch > 0)
  {
    switch (readch)
    {
      case '\r': // Ignore CR
        break;
      case '\n': // Return on new-line
        rpos = pos;
        pos = 0;  // Reset position index ready for next time
        return rpos;
      default:
        if (pos < len - 1)
        {
          buffer[pos++] = readch;
          buffer[pos] = 0;
        }
    }
  }
  return 0;
}


/*
     Read serial input and process the data
     source and more info: https://hackingmajenkoblog.wordpress.com/2016/02/01/reading-serial-on-the-arduino/
*/
bool serial_input()
{
  if (readline(Serial.read(), buf, input_buf_size) > 0)
  {
    return true;
  }
  else return false;
}

// filter category from user input
void fill_cat()
{
  // fill category var
  sscanf(buf, "%s ", cat);
}
/*
 * assign serial input parameters to variables
*/
void ADC_parse()
{
  sscanf(buf, "%s %d %d %d %d %d %d", cat, &analog_ports, &s_nbr, &s_rate, &trigger, &trigger_event, &debounce); // space delimited
}

void TEXT_parse()
{
  sscanf(buf, "%s %d %d %s", cat, &TEXT_col, &TEXT_row ,TEXT_text); // 
}

void ATT_parse()
{
  //sscanf(buf, "%s %d %d %s", cat, &TEXT_col, &TEXT_row ,TEXT_text); // 
}

void DAC_parse()
{
//  Serial.println(buf);
  sscanf(buf, "%s %s", cat, DAC_startstop); // space delimited
}


/*
     Prevent that critical parameters get out of range. Maximum values depent on hardware.
     Notice 'leaving nnnnnn bytes for local variables' after compiling for available mem space
     (every sample = 2 bytes)
*/
void check_parsed_input()
{
  s_nbr = min(s_nbr, s_nbr_max);                    // number of samples
  s_rate = min(s_rate, s_rate_max);                 // samples/sec
}
