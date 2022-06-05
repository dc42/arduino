#ifndef __PushButton_Included
#define __PushButton_Included

class PushButton
{
  bool state;
  bool newPress;
  int count;
  int pin;
  
public:
  PushButton(int p) : state(false), count(0), pin(p) {}
  void init();
  void poll();

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


