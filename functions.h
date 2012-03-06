void first_come_first_served();
void shortest_job_first();
void pre_shortest_job_first();
void round_robin();
void pre_priority();

void print_timestamp();
int random_time_amt();
int expo_random();

void populate_template_queue();
void reset_simulator();
void run_cpu_sim();

JobStatCollection get_stats(Job *);
void print_all_stats();

void process_submission(int);
void process_start(int);
void process_context_switch(int *, int);
void process_complete(int);
void process_stats(int);

void set_clock_to(int);
void increment_clock(int);

