#define _GNU_SOURCE //I'm not sure why I need this, but gcc won't link w/o it.
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include <constants.h>
#include <structs.h>
#include <functions.h>

//All times are in MILLISECONDS (as integers... so 10 = 10ms)

//We'll need to keep track of the jobs in the queue
//as well as their statistics
Job *QUEUE;
Job *TEMPLATE_QUEUE;
JobStatCollection **ALL_STATS;

//We'll also need some state variables for this simulation
int CURRENT_TIME = 0; //Time to get a watch... heh. SIGH -nz
int WAITING_PROCESSES = 0; //How many processes still have not finished?
int PERFORMED_CONTEXT_SWITCH = NO; //We should only incur the CS penalty after the first switch

/**
 * Main function... coordinate the start of the program
 */
int main(int argc, char **argv) {
	if (argc == 2 && strcmp(argv[1], "-PART2") == 0) {
		PART = 2;
	}
	QUEUE = calloc(TOTAL_PROCESSES, sizeof(Job));
	TEMPLATE_QUEUE = calloc(TOTAL_PROCESSES, sizeof(Job));
	ALL_STATS = calloc(5, sizeof(JobStatCollection *));
	int i;
	for (i = 0; i < 5; i++) {
		ALL_STATS[i] = calloc(TOTAL_PROCESSES, sizeof(JobStatCollection));
	}
	populate_template_queue();
	run_cpu_sim();
	return EXIT_SUCCESS;
}

/**
 * Copy the template queue into the working queue
 */
void reset_simulator() {
	memcpy(QUEUE, TEMPLATE_QUEUE, TOTAL_PROCESSES * sizeof(Job));
	CURRENT_TIME = 0;
	WAITING_PROCESSES = TOTAL_PROCESSES;
	PERFORMED_CONTEXT_SWITCH = NO;

	int i;
	for (i = 0; i < TOTAL_PROCESSES; i++) {
		//Print submission notices for the jobs submitted at time 0ms
		if (QUEUE[i].submit_time == 0) {
			process_submission(i);
		}	
	}
}

/**
 * Print out that nifty timestamp that Goldschmidt has in his example.
 */
void print_timestamp() {
	printf("[time %5dms] ", CURRENT_TIME);
}

/**
 * Give me a random number between 500 and 7500 (linear distribution)
 */
int random_time_amt() {
	return (((double) rand() / (double) RAND_MAX) * 7000) + 500;
}

/**
 * Give me a random priority (integer [1, 4])
 */
int random_priority() {
	int myRand = 5;
	while (myRand == 5) {
		myRand = (((double) rand() / (double) RAND_MAX) * 4) + 1;
	}
	//We'll only get 5 with a chance of 1/RAND_MAX... 
	// but we like to consider the edge cases.

	//SOMETIMES WE LIKE TO PLAY CHANCE GAMES
	//Get out, Noah
	return myRand;
}

/**
 * Return an exponentially-distributed random number.
 * Requirements for this function have been changed as of
 * Friday's lecture; see the project instructions for details.
 *
 * This function returns an expo-dist-rand in [0, 8000).
 */
int expo_random() {
	double base_rand = (double) rand() / (double) RAND_MAX;
	if (base_rand <= .25) {
		return 0;
	} else {
		//Shameless variable re-use here
		base_rand = (double) rand() / (double) RAND_MAX;
		int x = 9001;
		double lambda = 0.001;
		while (x > 8000) {
			x = -1 * (log(base_rand) / lambda);
		}
		return x;
	}
}

/**
 * Populate the template queue with actual jobs...
 * Part I: All jobs start at time=0
 * Part II: Some jobs start at random time
 */
void populate_template_queue() {
	int i;
	//srand(time(NULL));
	srand(1);
	for (i = 0; i < TOTAL_PROCESSES; i++) {
		//Give me a random number between 500 and 7500
		int job_length = random_time_amt();
		int submit_time = 0;
		if (PART == 2) { //Variable start times
			submit_time = expo_random();
		}

		//Make a new job object and throw it into the queue
		//  pid's start at 1 in goldschmidt's example... hence the i+1
		Job new_job = {i+1, job_length, random_priority(), submit_time, -1, -1, 0, JOB_NOT_STARTED};
		TEMPLATE_QUEUE[i] = new_job;
	}
}

/**
 * Start the correct CPU scheduling algorithm's function
 * This might be ugly, but that's life
 */
void run_cpu_sim() {
	if (SCHEDULING_METHOD == FIRST_COME_FIRST_SERVED) {
		first_come_first_served();
		print_all_stats();
	} else if (SCHEDULING_METHOD == SHORTEST_JOB_FIRST) {
		shortest_job_first();
		print_all_stats();
	} else if (SCHEDULING_METHOD == PRE_SHORTEST_JOB_FIRST) {
		pre_shortest_job_first();
		print_all_stats();
	} else if (SCHEDULING_METHOD == ROUND_ROBIN) {
		round_robin();
		print_all_stats();
	} else if (SCHEDULING_METHOD == PRE_PRIORITY) {
		pre_priority();
		print_all_stats();
	}	else if (SCHEDULING_METHOD == ALL) {
		first_come_first_served();
		shortest_job_first();
		pre_shortest_job_first();
		round_robin();
		pre_priority();

		int i;
		for (i = 0; i < 5; i++) {
			SCHEDULING_METHOD = i;
			print_all_stats();
		}
	} else {
 		fprintf(stderr, "Invalid Scheduling Algorithm\n");
	}
}

/**
 * First-come, first-served.  Pretty simple.
 */
void first_come_first_served() {
	SCHEDULING_METHOD = FIRST_COME_FIRST_SERVED;
	print_banner();
	reset_simulator();

	int current_proc_idx; //Index of current job object in queue (NOT PID)
	while (WAITING_PROCESSES > 0) { //We've still got jobs to take care of
		//Find a job to start
		int i;
		int min_time = INT_MAX; int min_idx = 0; //Guaranteed to be overwritten
		for (i = 0; i < TOTAL_PROCESSES; i++) {
			if (QUEUE[i].status == JOB_NOT_STARTED
			&& QUEUE[i].submit_time < min_time) {
				min_time = QUEUE[i].submit_time;
				min_idx = i;
			}
		}
		//So now we've got the next one in line, but what if it's still in the future?
		if (QUEUE[min_idx].submit_time > CURRENT_TIME) {
			set_clock_to(QUEUE[min_idx].submit_time);
		}

		process_context_switch(&current_proc_idx, min_idx);
		process_start(current_proc_idx);
	
		//Increment the timer by the amt of time required to complete
		increment_clock(QUEUE[current_proc_idx].time_required);

		process_complete(current_proc_idx);
	
	} //End while (num waiting processes > 0)
} //End first_come_first_served function

/**
 * At a given time, the processor picks the shortest job
 * in the ready queue and runs it to completion.
 */
void shortest_job_first() {
	SCHEDULING_METHOD = SHORTEST_JOB_FIRST;
	print_banner();
	reset_simulator();

	int current_proc_idx;
	while (WAITING_PROCESSES > 0) {
		//Find the shortest submitted job not yet started
		int i; 
		int min_job_length = INT_MAX; int min_job_idx = 0;
		for (i = 0; i < TOTAL_PROCESSES; i++) {
			if (QUEUE[i].status == JOB_NOT_STARTED
			    && QUEUE[i].submit_time <= CURRENT_TIME
			    && QUEUE[i].time_required < min_job_length) {
				min_job_length = QUEUE[i].time_required;
				min_job_idx = i;
			}
		}

		//So now we've got the shortest submitted job not yet started.
		process_context_switch(&current_proc_idx, min_job_idx);
		process_start(current_proc_idx);

		increment_clock(QUEUE[current_proc_idx].time_required);

		process_complete(current_proc_idx);
	} //End while num waiting processes > 0
}

/**
 * The processor runs the shortest job in the ready queue.
 * If a job is submitted that requires a smaller amount of time
 * than the currently-running process needs to complete its work,
 * the currently-running process is preempted (replaced).
 */
void pre_shortest_job_first() {
	SCHEDULING_METHOD = PRE_SHORTEST_JOB_FIRST;
	print_banner();
	reset_simulator();


	int current_proc_idx;
	while (WAITING_PROCESSES > 0) {
		//We're at time T, and we're looking for a process to do work on.
		//Find the minimum amount of work I can do of all submitted jobs.
		int i;
		int min_work_required = INT_MAX; int min_work_req_idx = 0;
		for (i = 0; i < TOTAL_PROCESSES; i++) {
			if (QUEUE[i].status != JOB_COMPLETED
			    && QUEUE[i].submit_time <= CURRENT_TIME
					&& QUEUE[i].time_required - QUEUE[i].time_spent < min_work_required) {
				min_work_required = QUEUE[i].time_required - QUEUE[i].time_spent;
				min_work_req_idx = i;
			}
		}

		//Now we've got a job selected that we should work on.
		//Proceed until the next job is submitted...
		// at that point, we select the shortest job again,
		// and context switch if necessary.

		process_context_switch(&current_proc_idx, min_work_req_idx);
		process_start(current_proc_idx);

		//Find the next job that might preempt the currently running job
		//It's got to be submitted in the future... if it were submitted already,
		//then that means we've already decided NOT to choose it, so there's
		//no way it would preempt something that WAS chosen... I think.
		
		//^ This makes perfect sense. Its strictly decreasing - if x is shorter than y and z is shorter than x
		//then z is shorter than y
		int next_start_time = INT_MAX;
		for (i = 0; i < TOTAL_PROCESSES; i++) {
			if (QUEUE[i].submit_time > CURRENT_TIME
			    && QUEUE[i].submit_time < next_start_time) {
				next_start_time = QUEUE[i].submit_time;
			}
		}
		//Will this job finish before the next job is submitted?
		int currentTimeRemaining = QUEUE[current_proc_idx].time_required - QUEUE[current_proc_idx].time_spent;
		if (next_start_time > (CURRENT_TIME + currentTimeRemaining)) {
			//We can finish this job!
			increment_clock(currentTimeRemaining);
			process_complete(current_proc_idx);
		} else {
			//We can't finish this job, so do as much as we can.
			QUEUE[current_proc_idx].time_spent += (next_start_time - CURRENT_TIME);
			set_clock_to(next_start_time);
		}
	} //End while num waiting processes > 0	*/
}

/**
 * The processor gives an equal amount of time to all jobs
 * in the ready queue.
 * This time slice is predetermined and constant.
 * If you're "late" to the ready queue (as in, we start a new round and THEN you get submitted), we pass over you until next round
 * This prevents jobs from just piling up in the queue and preventing jobs who were there already from being 'ignored', so to speak
 */
void round_robin() {
	SCHEDULING_METHOD = ROUND_ROBIN;
	print_banner();
	reset_simulator();

	int current_proc_idx;
	while(WAITING_PROCESSES > 0) {
		int i;
		int theParty = CURRENT_TIME; //Life's a party!
		for(i = 0; i < TOTAL_PROCESSES; i++) {
			if (QUEUE[i].submit_time <= theParty //But hey, if you're late to the party, we understand. We'll get you on the next round!
			    && QUEUE[i].status != JOB_COMPLETED) {
				
				process_context_switch(&current_proc_idx, i);
				process_start(current_proc_idx);
			
				int currentTimeRemaining = QUEUE[current_proc_idx].time_required - QUEUE[current_proc_idx].time_spent;
				if(currentTimeRemaining < ROUND_ROBIN_TIME_SLICE) //if you're going to be done partying after this round...
				{
					increment_clock(currentTimeRemaining);
					process_complete(current_proc_idx);
				} else {                                          //otherwise, do what you can and it's someone else's turn!
					QUEUE[current_proc_idx].time_spent += ROUND_ROBIN_TIME_SLICE;
					increment_clock(ROUND_ROBIN_TIME_SLICE);
				} 
			}
		}
	}
}

/**
 * The processor runs the highest-priority job in the ready queue.
 * If a job is submitted that has a higher priority
 * than the currently-running process has,
 * the currently-running process is preempted (replaced).
 */
void pre_priority() {
	SCHEDULING_METHOD = PRE_PRIORITY;
	print_banner();
	reset_simulator();

	int current_proc_idx;
	while(WAITING_PROCESSES > 0) {
		int i;
		int highest_priority = INT_MAX;
		int highest_priority_idx;
		for(i = 0; i < TOTAL_PROCESSES; i++) {
			if(QUEUE[i].submit_time <= CURRENT_TIME
			&& QUEUE[i].status != JOB_COMPLETED
			&& QUEUE[i].priority < highest_priority) {
				highest_priority = QUEUE[i].priority;
				highest_priority_idx = i;
			} else if (QUEUE[i].submit_time <= CURRENT_TIME
			           && QUEUE[i].status != JOB_COMPLETED
								 && QUEUE[i].priority == highest_priority
								 && QUEUE[i].submit_time < QUEUE[highest_priority_idx].submit_time) {
				highest_priority_idx = i;
			}
		}

		process_context_switch(&current_proc_idx, highest_priority_idx);
		process_start(current_proc_idx);

		int next_start_time = INT_MAX;
		for (i = 0; i < TOTAL_PROCESSES; i++) {
			if (QUEUE[i].submit_time > CURRENT_TIME
			    && QUEUE[i].priority < highest_priority) {
				next_start_time = QUEUE[i].submit_time;
			}
		}

		int currentTimeRemaining = QUEUE[current_proc_idx].time_required - QUEUE[current_proc_idx].time_spent;
		if (next_start_time > (CURRENT_TIME + currentTimeRemaining)) {
			increment_clock(currentTimeRemaining);
			process_complete(current_proc_idx);
		} else {
			QUEUE[current_proc_idx].time_spent += (next_start_time - CURRENT_TIME);
			set_clock_to(next_start_time);
		}
	}
}

/**
 * Advance the clock a specified amount;
 * Print the job submission notices in-between, though.
 * amount: The amount by which we should increment the clock
 */
void increment_clock(int amount) {
	//New jobs could be submitted during this increment...
	//Increment up through job submissions

	while (amount > 0) {
		int min_submit_time = INT_MAX; int min_submit_idx = 0;
		int i;
		for (i = 0; i < TOTAL_PROCESSES; i++) {
			if (QUEUE[i].submit_time > CURRENT_TIME
					&& QUEUE[i].submit_time < min_submit_time) {
				min_submit_time = QUEUE[i].submit_time;
				min_submit_idx = i;
			}
		}

		if (min_submit_time - CURRENT_TIME <= amount) {
			//We want to increment the clock past the next submit
			amount -= min_submit_time - CURRENT_TIME;
			CURRENT_TIME = min_submit_time;
			process_submission(min_submit_idx);
		} else {
			//amount < min_submit_time - CURRENT_TIME
			//We can increment the clock without worrying about the next submission
			CURRENT_TIME += amount;
			amount = 0;
		}
	}
}

/**
 * Advance the clock to a specific time;
 * Print the job submission notices in-between.
 * to_time: The time to which to set the clock
 */
void set_clock_to(int to_time) {
	increment_clock(to_time - CURRENT_TIME);
}

/**
 * Print a notice when a process is submitted.
 * q_idx: The index in the queue of the submitted job
 */
void process_submission(int q_idx) {
	print_timestamp();
	printf("Process %2d created (requires %4dms CPU time, priority %d)\n", 
	  QUEUE[q_idx].pid, QUEUE[q_idx].time_required, QUEUE[q_idx].priority );
}

/**
 * A process is just starting... do some starting logic
 * q_idx: The index in the queue of the newly starting job
 */
void process_start(int q_idx) {
	if (QUEUE[q_idx].status != JOB_NOT_STARTED) {
		return;
	}
	print_timestamp();
	printf("Process %2d accessed CPU for the first time", QUEUE[q_idx].pid);
	printf(" (initial wait time %dms)\n", CURRENT_TIME - QUEUE[q_idx].submit_time);
	QUEUE[q_idx].start_time = CURRENT_TIME;
	QUEUE[q_idx].status = JOB_IN_PROGRESS;
}

/**
 * Perform a context switch (incur a delay and change the variable)
 * cur_idx: The index in the queue of the current process
 * new_idx: The index in the queue of the replacing process
 */
void process_context_switch(int *cur_idx, int new_idx) {
	//If the first process is preempted before it finishes, we should still
	// do a context switch; the problem is that we don't decrement
	// WAITING_PROCESSES until a process FINISHES.
	// I guess, for now, we'll use a flag to tell us if we've done a CS before.
	if (PERFORMED_CONTEXT_SWITCH == YES) {
		if (*cur_idx == new_idx) { //No switch necessary
			return;
		}
		//Write a context switch message and add the time penalty
		print_timestamp();
		printf("Context switch (swapped out process %d for process %d)\n",
			QUEUE[*cur_idx].pid, QUEUE[new_idx].pid);
		increment_clock(CONTEXT_SWITCH_PENALTY);
	}
	*cur_idx = new_idx;
	PERFORMED_CONTEXT_SWITCH = YES;
}


/**
 * A process has completed... do some completion logic
 * q_idx: The index in the queue of the newly completed job
 */
void process_complete(int q_idx) {
	QUEUE[q_idx].end_time = CURRENT_TIME;
	QUEUE[q_idx].status = JOB_COMPLETED;
	WAITING_PROCESSES--;
	process_stats(q_idx);
}

/**
 * After a process has been completed, gather and store some stats
 * q_idx: The index in the queue of the job to analyze
 */
void process_stats(int q_idx) {
	JobStatCollection stats = get_stats(&QUEUE[q_idx]);
	ALL_STATS[SCHEDULING_METHOD][q_idx] = stats;
	print_timestamp();
	printf("Process %2d completed its CPU burst ", QUEUE[q_idx].pid);
	printf("(turnaround time %dms, ", stats.turnaround_time);
	printf("initial wait time %dms, ", stats.initial_wait_time);
	printf("total wait time %dms)\n", stats.total_wait_time);
}

/**
 * Print a pretty banner for the current algo so that we know what's going on
 */
void print_banner() {
	printf("==========================================================\n");
	if (SCHEDULING_METHOD == FIRST_COME_FIRST_SERVED) {
		printf("================= FIRST COME FIRST SERVED ================\n");
	} else if (SCHEDULING_METHOD == SHORTEST_JOB_FIRST) {
		printf("================= SHORTEST JOB FIRST =====================\n");
	} else if (SCHEDULING_METHOD == PRE_SHORTEST_JOB_FIRST) {	
		printf("============= PREEMPTIVE SHORTEST JOB FIRST ==============\n");
	} else if (SCHEDULING_METHOD == ROUND_ROBIN) {
		printf("===================== ROUND ROBIN ========================\n");
	} else if (SCHEDULING_METHOD == PRE_PRIORITY) {
		printf("===================== PREEMPTIVE PRIORITY ================\n");
	}
	printf("==========================================================\n");
}
	

JobStatCollection get_stats(Job *job) {
	JobStatCollection stats;
	stats.turnaround_time = job->end_time - job->submit_time;
	stats.initial_wait_time = job->start_time - job->submit_time;
	stats.total_wait_time = stats.turnaround_time - job->time_required;
	//printf("Job PID %d:\n", job->pid);
	//printf("  Submit Time: %dms\n", job->submit_time);
	//printf("  Start Time: %dms\n", job->start_time);
	//printf("  End Time: %dms\n", job->end_time);
	return stats;
}

void print_all_stats() {
	print_banner();

	int min_turnaround = ALL_STATS[SCHEDULING_METHOD][0].turnaround_time;
	int max_turnaround = ALL_STATS[SCHEDULING_METHOD][0].turnaround_time;
	int sum_turnaround = 0;
	double avg_turnaround = 0;

	int min_initial_wait = ALL_STATS[SCHEDULING_METHOD][0].turnaround_time;
	int max_initial_wait = ALL_STATS[SCHEDULING_METHOD][0].turnaround_time;
	int sum_initial_wait = 0;
	double avg_initial_wait = 0;

	int min_total_wait = ALL_STATS[SCHEDULING_METHOD][0].turnaround_time;
	int max_total_wait = ALL_STATS[SCHEDULING_METHOD][0].turnaround_time;
	int sum_total_wait = 0;
	double avg_total_wait = 0;

	int i;
	for (i = 0; i < TOTAL_PROCESSES; i++) {
		JobStatCollection cur = ALL_STATS[SCHEDULING_METHOD][i];
		//Turnaround time
		if (cur.turnaround_time < min_turnaround) { 
			min_turnaround = cur.turnaround_time; 
		}
		if (cur.turnaround_time > max_turnaround) {
			max_turnaround = cur.turnaround_time;
		}
		sum_turnaround += cur.turnaround_time;

		//Initial Wait time
		if (cur.initial_wait_time < min_initial_wait) {
			min_initial_wait = cur.initial_wait_time;
		}
		if (cur.initial_wait_time > max_initial_wait) {
			max_initial_wait = cur.initial_wait_time;
		}
		sum_initial_wait += cur.initial_wait_time;

		//Total Wait time
		if (cur.total_wait_time < min_total_wait) {
			min_total_wait = cur.total_wait_time;
		} 
		if (cur.total_wait_time > max_total_wait) {
			max_total_wait = cur.total_wait_time;
		}
		sum_total_wait += cur.total_wait_time;
	}
	
	avg_turnaround = (double) sum_turnaround / (double) TOTAL_PROCESSES;
	avg_initial_wait = (double) sum_initial_wait / (double) TOTAL_PROCESSES;
	avg_total_wait = (double) sum_total_wait / (double) TOTAL_PROCESSES;

	printf("\nTurnaround Time\n");
	printf("  Min: %6.3lfs, Max: %6.3lfs, Avg: %6.3lfs\n",
		min_turnaround / 1000.0, max_turnaround / 1000.0, avg_turnaround / 1000.0);

	printf("Initial Wait Time\n");
	printf("  Min: %6.3lfs, Max: %6.3lfs, Avg: %6.3lfs\n",
		min_initial_wait / 1000.0, max_initial_wait / 1000.0, avg_initial_wait / 1000.0);
	
	printf("Total Wait Time\n");
	printf("  Min: %6.3lfs, Max: %6.3lfs, Avg: %6.3lfs\n",
		min_total_wait / 1000.0, max_total_wait / 1000.0, avg_total_wait / 1000.0);
}
