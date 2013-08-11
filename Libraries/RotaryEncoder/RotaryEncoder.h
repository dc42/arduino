#ifndef __RotaryEncoderIncluded
#define __RotaryEncoderIncluded

class RotaryEncoder
{
  unsigned int state;
  int pin0, pin1;
  int ppc;
  int change;

  unsigned int readState()
  {
    return (digitalRead(pin0) ? 1u : 0u) | (digitalRead(pin1) ? 2u : 0u);
  }

public:
  RotaryEncoder(int p0, int p1, int pulsesPerClick) : 
    pin0(p0), pin1(p1), ppc(pulsesPerClick), change(0), state(0) {}

  void init();
  void poll();
  int getChange();
};

#endif


