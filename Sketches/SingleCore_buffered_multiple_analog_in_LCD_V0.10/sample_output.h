/*
    write content of s_array to Python/console
*/
void dump_sample_array()
{
  //Serial.println(s_nbr);
  for (int i = 0; i < s_nbr; i++)
  {
    //Serial.println(s_array[i]);               // for debugging
    Serial.write((s_array[i] >> 8) & 0xff);     // write MSB to Python/console
    Serial.write( s_array[i] & 0xff);           // write LSB to Python/console
  }
  //Serial.println("--------------");
}
