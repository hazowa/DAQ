void cat_ADC()
{
  ADC_parse();                // fill all variabel with user input
  check_parsed_input();       // prevent overflow
  show_parsed_input();        // in case debug is true, show input values on console


  if (LCD) LCD_show_parsed_input();// in case LCD is true, show input values on LCD display

  if (trigger == 1) TriggerEvent(trigger_port, trigger_event, debounce);

  digitalWrite(LED, LOW);     // to indicate sampling has started, dim LED
  get_samples();              // get analog samples, parameters in
  digitalWrite(LED, HIGH);    // to indicate sampling has stopped, turn on LED

  dump_sample_array();        // return samples to Python serial or console
  if (LCD) LCD_level_redraw = true;  //rebuild LCD level indicator
}
