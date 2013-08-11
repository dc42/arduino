#include "arduino.h"
#include "Scheduler.h"

// Set USE_TIMER2 nonzero to use timer 2 as the scheduler tick source.
// This alows us to generate a tick interval of just over 1ms whether using an 8MHz or 16MHz processor.
// PWM using the Timer2 registers is still possible.
// Set USE_TIMER2 zero to use timer1 as the tick source.
// This requires patching wiring.c to allow us to intercept the tick interrupt.
// The tick rate interval will be just over 1ms using a 16MHz clock, and just over 2ms using an 8MHz clock.
#define USE_TIMER2    (1)

#if USE_TIMER2

// We set the prescaler to 64 for 10MHz clock and above, and to 32 for < 10MHz clock
# if F_CPU >= 10000000
#  define TCCR2_PRESCALER  (64)
# else
#  define TCCR2_PRESCALER  (32)
# endif
# define TICKS_PER_SECOND  ((F_CPU) / (256 * TCCR2_PRESCALER))

#else

// Wiring.c always uses a prescaler of 64 (timer 0 doesn't support 32)
# define TICKS_PER_SECOND  ((F_CPU) / (256 * 64))
extern void (*tick_callback)();    // this has to be patched in to wiring.c

#endif

// Static data
Task * volatile Task::rlr = 0;
Task * volatile Task::dlr = 0;

Task::Task() : ticksToWakeup(-1), state(suspended), next(0) 
{
}

#ifdef USE_TIMER2
SIGNAL(TIMER2_OVF_vect)
{
  Task::tick();
}
#endif

void Task::init()
{
  uint8_t oldSREG = SREG;
  cli();
#if USE_TIMER2
  TCCR2A |= 0x03;                      // fast PWM mode
# if TCCR2_PRESCALER == 64
  TCCR2B = (TCCR2B & 0xC0) | 0x04;     // set prescaler to 64
# elif TCCR2_PRESCALER == 32
  TCCR2B = (TCCR2B & 0xC0) | 0x03;     // set prescaler to 32
# else
#  error "Invalid value for TCCR2_PRESCALER"
# endif
  TIMSK2 = 0x01;                       // enable interrupt on overflow
  ASSR = 0;                            // use internal clock
#else
  tick_callback = &Task::tick;
#endif  
  SREG = oldSREG;
}

const unsigned int Task::ticksPerSecond = TICKS_PER_SECOND;

void Task::wakeup(int sleepTime)
{
  uint8_t oldSREG = SREG;
  cli();
  if (state == suspended)
  {
    doWakeup(sleepTime);
  }
  SREG = oldSREG;
}

// This function may only be called with interrupts disabled
void Task::doWakeup(int sleepTime)
{
  next = (Task*)0;
  if (sleepTime == 0)
  {
    // Put task at end of ready list
    state = ready;
    ticksToWakeup = 0;
    Task** p = const_cast<Task**>(&rlr);
    while (*p != 0)
    {
      p = const_cast<Task**>(&((*p)->next));
    }
    *p = this;
  }
  else if (sleepTime > 0)
  {
    // Insert task into delay list
    state = delaying;
    next = (Task*)0;
    Task** p = const_cast<Task**>(&dlr);
    while (*p != 0)
    {
      Task* t = *p;
      if (t->ticksToWakeup > sleepTime)
      {
        t->ticksToWakeup -= sleepTime;
        next = t;
        break;
      }
      sleepTime -= t->ticksToWakeup;
      p = const_cast<Task**>(&(t->next));
    }
    ticksToWakeup = sleepTime;
    *p = this;
  }
}


void Task::suspend()
{
  switch(state)
  {
  case suspended:
    break;

  case ready:
    if (this != rlr)    // can't suspend current task
    {
      Task** p = const_cast<Task**>(&rlr);
      uint8_t oldSREG = SREG;
      cli();
      while (*p != 0)
      {
        if (*p == this)
        {
          *p = next;
          break;
        }
        p = const_cast<Task**>(&((*p)->next));
      }
      state = suspended;
      SREG = oldSREG;
    }
    break;

  case delaying:
    {
      Task** p = const_cast<Task**>(&dlr);
      uint8_t oldSREG = SREG;
      cli();
      while (*p != 0)
      {
        if (*p == this)
        {
          *p = const_cast<Task*>(next);
          if (next != 0)
          {
            next->ticksToWakeup += ticksToWakeup;
          }
          break;
        }
        p = const_cast<Task**>(&((*p)->next));
      }
      state = suspended;
      SREG = oldSREG;
      ticksToWakeup = 0;
    }
    break;
  }
}

// Suspend all tasks other than this one.
void Task::suspendOthers()
{
  uint8_t oldSREG = SREG;
  cli();      // disable interrupts to prevent tasks on the DLR being woken up
  // Suspend all tasks on the delay list
  Task *t = dlr;
  while (t != 0)
  {  
    t->state = suspended;
    t = t->next;
  }
  dlr = 0;
  
  // Suspend all tasks on the ready list except the current one
  t = rlr;
  if (t != 0)
  {
    while ((t = t->next) != 0)
    {
      t->state = suspended;
    }
    rlr->next = 0;
  }
  SREG = oldSREG;
}

void Task::loop()
{
  uint8_t oldSREG = SREG;
  cli();
  Task* current = rlr;
  SREG = oldSREG;
  if (current != 0)
  {
    int t = current->body();
    uint8_t oldSREG = SREG;
    cli();
    current->state = suspended;
    rlr = current->next;
    if (t >= 0)
    {
      current->doWakeup(t);
    }
    SREG = oldSREG;
  }
}

// Tick ISR, must be called with interrupts already disabled
void Task::tick()
{
  Task* t = dlr;
  if (t != 0)
  {
    --(t->ticksToWakeup);
    if (t->ticksToWakeup == 0)
    {
      // Remove this task any any more with zero ticks to wakeup from the delay list
      t->state = ready;
      Task** tt = const_cast<Task**>(&(t->next));
      while (*tt != 0 && (*tt)->ticksToWakeup == 0)
      {
        (*tt)->state = ready;
        tt = const_cast<Task**>(&((*tt)->next));
      }
      dlr = *tt;
      *tt = 0;
      
      // Append these tasks to the ready list
      Task** pp = const_cast<Task**>(&rlr);
      while (*pp != 0)
      {
        pp = const_cast<Task**>(&((*pp)->next));
      }
      *pp = t;
    }
  } 
}

// End


