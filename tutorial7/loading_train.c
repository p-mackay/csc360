#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define NANOSECOND_CONVERSION 1e9

#define NUM_TRAINS 5
pthread_mutex_t start_timer;
pthread_cond_t train_ready_to_load;
bool ready_to_load = false;

struct timespec start_time = { 0 };

// Convert timespec to seconds
double timespec_to_seconds(struct timespec *ts)
{
	return ((double) ts->tv_sec) + (((double) ts->tv_nsec) / NANOSECOND_CONVERSION);
}

// Thread function
void* train_thread(void *t)
{
	size_t i = (intptr_t) t;
	
	// Wait until start signal given
	pthread_mutex_lock(&start_timer);
	while (!ready_to_load)
	{
		pthread_cond_wait(&train_ready_to_load, &start_timer);
	}
	pthread_mutex_unlock(&start_timer);
	
	struct timespec load_time = { 0 };
	clock_gettime(CLOCK_MONOTONIC, &load_time);
	
	printf("Start loading train %ld at time %f (at simulation time %f) \n",
		i,
		timespec_to_seconds(&load_time),
		timespec_to_seconds(&load_time) - timespec_to_seconds(&start_time)
	);

	pthread_exit(NULL);
}

void start_trains(void)
{
	pthread_mutex_lock(&start_timer);
	ready_to_load = true;
	pthread_cond_broadcast(&train_ready_to_load);
	pthread_mutex_unlock(&start_timer);
}

int main(void)
{
	// Trains
	pthread_t tid[NUM_TRAINS];

	struct timespec initial_time = { 0 };
	clock_gettime(CLOCK_MONOTONIC, &initial_time);
	printf("Program start time: %f\n", timespec_to_seconds(&initial_time));

	struct timespec create_time = { 0 };
	for(size_t i = 0; i < NUM_TRAINS; ++i)
	{
		clock_gettime(CLOCK_MONOTONIC, &create_time);
		printf("Creating train %ld at time %f\n", i, timespec_to_seconds(&create_time));

		// Create train thread
		pthread_create(&tid[i], NULL, train_thread, (void *) (intptr_t) i);
		sleep(1); // Simulate delay
	}

	// Node: Don't synchronize your broadcast this way in your own code.
	sleep(1);

	// Get start time
	clock_gettime(CLOCK_MONOTONIC, &start_time);
	printf("Program start time: %f\n", timespec_to_seconds(&start_time));

	// Start trains loading!
	start_trains();

	for(size_t i = 0; i < NUM_TRAINS; ++i)
	{
		pthread_join(tid[i], NULL);
	}

	return 0;
}