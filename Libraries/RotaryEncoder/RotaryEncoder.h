#ifndef __RotaryEncoderIncluded
#define __RotaryEncoderIncluded

class RotaryEncoder
{
  unsigned int state;
  int pin0, pin1;
  int ppc;
  int change;

  unsigned int readState();

public:
  RotaryEncoder(int p0, int p1, int pulsesPerClick) : 
    state(0), pin0(p0), pin1(p1), ppc(pulsesPerClick), change(0) {}

  void init();
  void poll();
  int getChange();
};

#endif


