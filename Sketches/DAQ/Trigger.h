// trigger on/off and event
int trigger_port = 27;              // port 27 on esp32 defined as trigger port
int trigger = 0;                    // when '1' wait for event on port 27 to start conversion, '0' ignore
#define Low 0                       // 0 = Low (trigger when pin is LOW)
#define High 1                      // 1 = High (trigger when pin is HIGH)
#define Change 2                    // 2 = Change ( trigger when the pin changes value from LOW to HIGH or HIGH to LOW)
#define Rising 3                    // 3 = Rising (trigger when the pin goes from LOW to HIGH)
#define Falling 4                   // 4 = Falling (trigger when the pin goes from HIGH to LOW)
#define Ignore 99                   // 99 = Ignore trigger
int trigger_event = Ignore;         // default trigger level/edge is low. See setup(), if pullup or pulldown resistor is active
unsigned int debounce = 100;        // Time in microseconds a level must be stable to prevent jitter




/*
   Wait till port is stable during al least the specified debounce time (in microsec)
*/
void TriggerDebounce(int port, bool target, unsigned int debtime)
{
  int future_time = micros() + debtime;
  do
  {
    // every time port value changes, reset future_time
    if (digitalRead(port) != target)  future_time = micros() + debtime;
  } while (micros() > future_time);
}


/* UITZOEKEN OF DEZE ROUTINE HANDIGER OP INTERRUPT BASIS KAN
    poll triggerport until the specified event occurs
  //    0 = Low (trigger whenever pin is LOW)
  //    1 = High (trigger whenever pin is HIGH)
  //    2 = Change ( trigger whenever the pin changes value from LOW to HIGH or HIGH to LOW)
  //    3 = Rising (trigger when the pin goes from LOW to HIGH)
  //    4 = Falling (trigger when the pin goes from HIGH to LOW)
*/
void TriggerEvent(int tp, int te, unsigned int debounce_time)      //tp = triggerport, te =trigger event, deb = debounce time (microsec)
{
  bool port_val;
  switch (te)
  {
    case Low:
      {
        while (digitalRead(tp) == HIGH) {};
        if (debounce_time > 0 ) TriggerDebounce(tp, LOW, debounce_time); // if debounce time greater then 0, wait till port stays low during debounce time
        break;
      }
    case High:
      {
        while (digitalRead(tp) == LOW) {};
        if (debounce_time > 0 ) TriggerDebounce(tp, HIGH, debounce_time); // if debounce time greater then 0, wait till port stays low during debounce time
        break;
      }
    case Change:
      {
        do
        {
          port_val = digitalRead(tp);
        } while (digitalRead(tp) == port_val);
        break;
      }
    case Rising:
      {
        while (digitalRead(tp) == LOW) {};         // wait until port is LOW
        while (digitalRead(tp) == HIGH) {};        // when LOW wait for edge to HIGH
        break;
      }
    case Falling:
      {
        while (digitalRead(tp) == HIGH) {};        // wait until port is LOW
        while (digitalRead(tp) == LOW) {};         // when LOW wait for edge to HIGH
        break;
      }
    default:
        break;
  }
}
