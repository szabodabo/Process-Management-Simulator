/**
 * These options dictate the scheduling algorithm that
 * the simulator will emulate.
 * This option is set with the SCHEDULING_METHOD variable.
 */
#define FIRST_COME_FIRST_SERVED 0
#define SHORTEST_JOB_FIRST 1
#define PRE_SHORTEST_JOB_FIRST 2
#define ROUND_ROBIN 3
#define PRE_PRIORITY 4
#define ALL 5

/**
 * Just some helpful status constants
 */
#define JOB_NOT_STARTED 0
#define JOB_IN_PROGRESS 1
#define JOB_COMPLETED 2
#define YES 1
#define NO 0


/**
 * These settings control the behavior of the simulator.
 */
int SCHEDULING_METHOD = ALL; //See above options
int TOTAL_PROCESSES = 20; //This is the N value
int PART = 1; //Part I or Part II
int CONTEXT_SWITCH_PENALTY = 12; //This is "Tcs"
int ROUND_ROBIN_TIME_SLICE = 100; //Fairly (ha) self-explanatory
int RANDOMIZE = YES; //Change to 'NO' if we're using a constant seed (and not current time)
