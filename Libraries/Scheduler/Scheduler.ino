// Demo function for task scheduler
// This demo uses two intstances of SimpleTask to blink 2 LEDs at different rates.
// A better approach (avoiding use of global variables) would be to derive a LED blink class from Task instead.

#include "Scheduler.h"

bool ledState1 = false;
bool ledState2 = false;

// This task blinks a led on pin 13, 200ms on, 200ms off
int myTaskFunc1()
{
  ledState1 = !ledState1;
  digitalWrite(13, ledState1 ? HIGH : LOW);
  return 200;
}

// This task blinks a led on pin 12, 500ms on, 500ms off
int myTaskFunc2()
{
  ledState2 = !ledState2;
  digitalWrite(12, ledState2 ? HIGH : LOW);
  return 500;
}

// Declare a SimpleTask to execute each task function
SimpleTask myTask1(&myTaskFunc1);
SimpleTask myTask2(&myTaskFunc2);

void setup()
{
  pinMode(13, OUTPUT);
  pinMode(12, OUTPUT);
  Task::init();        // initialize the task subsystem
  myTask1.start(0);    // start the first task immediately
  myTask2.start(0);    // start the second task immediately
}

void loop()
{
  Task::loop();
}

// End

