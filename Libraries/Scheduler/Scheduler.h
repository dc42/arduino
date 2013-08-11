// Scheduler for Arduino

class Task
{
  enum TaskState
  {
    suspended = 0,
    ready = 1,
    delaying = 2
  };

public:
  Task();	                            // build a new task but don't start it
  void suspend();			    // suspend the task (i.e. cancel any scheduled wakeup). If the task is already executing then the call is ignored.
  static void suspendOthers();              // suspend all other tasks except the current one
  
  // Test whether the task is suspended
  bool isSuspended() const {
    return state == suspended;
  }

  static void loop();			    // this is the function we must keep calling for the scheduler to run
  static void init();
  static void tick();

  static const unsigned int ticksPerSecond;
  
protected:
  void wakeup(int sleepTime);	            // wake up a task after the specified time. Safe to call from an ISR.
  
  // This function is the body of the task.
  // The integer return value is the number of ticks we want to elapse before we are woken up again.
  // If it is zero then the task is placed at the back of the ready queue.
  // A negative number means we want to be suspended until another task wakes us up.
  virtual int body() = 0;
  
private:

  // internal function to wake up a task, must be called with interruopts disabled and task in suspended state
  void doWakeup(int sleepTime);	

  int ticksToWakeup;		            // if this task is the first on on the delay list, then this is the number of ticks until it gets scheduled.
                                            // if it is not the first task on the delay list, need to add the tick counts from all other tasks ahead of it.
  volatile TaskState state;	            // what state the task is in
  Task * volatile next;		            // link to next task in list

  static Task * volatile rlr;		    // ready list root
  static Task * volatile dlr;	            // delay list root
};

class SimpleTask : public Task
{
public:
  // Definition of the type of a SimpleTask function.
  // The integer return value is the number of ticks we want to elapse before we are woken up again.
  // A negative number means we want to be suspended until another task wakes us up.
  typedef int (*TaskFunc)();

  SimpleTask(TaskFunc f) : Task(), func(f) {}	 // build a new SimpleTask but don't start it

  void start(int sleepTime)
  {
    wakeup(sleepTime);
  }
  
protected:
  /*override*/ int body()
  {
    return func();
  }

private:  
  TaskFunc func;
};

// End

