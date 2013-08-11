arduino
=======

This repository contains reusable modules, drivers and patches for the Arduino platform. Here is a description of the
projects:

Scheduler
=========
This is a cooperative multi-tasking scheduler for the Arduino. Conventionally, if you need to control and/or monitor
several devices concurrently, then you need to do this in the Arduino loop() function. The code for all the devices is mixed
up, making it difficult to understand and hard to maintain. In contrast, if you use the task scheduler, you can write
the code for each device separately, then just create a task for each device to run its code.

Limitations of the task scheduler:

1. The scheduler is based on a regular tick (normally at 1ms intervals), so any time intervals between things happening
must either be much less that 1ms, or must be multiples of 1ms, or must be specially programmed using one of the
hardware timers.

2. You must write the code for each task so that it executes in much less than 1ms before it returns, so that other
tasks can run. Anything that takes longer must be broke down into smaller steps.

3. You cannot call any library functions that may take more than a millisecond to execute. Library functions that wait
for things to complete need to be rewritten to use the task scheduler instead.

Lcd7920
=======
This is a simple library to drive 128x64 graphic LCDs based on the ST7920 chip in serial mode, thereby using only two
Arduino pins (preferably the MOSI and SCLK pins to get best speed). It is smaller but less comprehensive than U8glib.

PushButton
==========
This is a class to read and debounce a push button. It can be used in conjunction with the task scheduler, or without
if you arrange for your code to call the poll() function at regular intervals.

RotaryEncoder
=============
This is a class to read rotary encoders, allowing for contact bounce and variation in the detent position. It can be
used with or without the task scheduler. To maintain responsiveness, the poll() function must be called at intervals
of 1ms or 2ms.
