#ifndef __PushButton_Included
#define __PushButton_Included

class PushButton
{
  bool state;
  bool newPress;
  int count;
  int pin;
  
public:
  PushButton(int p) : pin(p) {}
  void init();
  void poll();

  // Return the debounced state
  bool getState() 
  { 
    return state; 
  }

  bool getNewPress() 
  {
    if (newPress)
    {
      newPress = false;
      return true;
    }
    return false;
  }
};

#endif


