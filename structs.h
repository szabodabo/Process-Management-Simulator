typedef struct {
  int pid;
  int time_required;
  int priority; //Not used for all algorithms
  int submit_time; //Either 0 or delayed start
  int start_time;  //Set to -1 initially
  int end_time; //Set to -1 initially
  int time_spent; //Amount of time spent so far
  int status; //Constants define this one
} Job;

typedef struct {
  int turnaround_time;
  int initial_wait_time;
  int total_wait_time;
} JobStatCollection;

