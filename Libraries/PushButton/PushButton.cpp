#include "PushButton.h"
#include "arduino.h"

static const int debounceCount = 10;

void PushButton::init()
{
  pinMode(pin, INPUT_PULLUP);
  count = 0;
  state = false;
  newPress = false; 
}
  
// Call this every 1ms or so
void PushButton::poll()
{
  bool b = !digitalRead(pin);
  if (b != state)
  {
    ++count;
    if (count == debounceCount)
    {
      state = b;
      count = 0;
      if (state)
      {
        newPress = true;
      }
    }
  }
  else if (count > 0)
  {  
    --count;
  }
}

// End

