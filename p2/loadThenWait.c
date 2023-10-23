#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_TRAINS 3
#define LOADING_TIME 1
#define CROSSING_TIME 2

pthread_mutex_t station_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t start_loading_condition = PTHREAD_COND_INITIALIZER;
pthread_cond_t crossing_condition = PTHREAD_COND_INITIALIZER;

typedef struct {
    int number;
} Train;

Train trains[NUM_TRAINS];
pthread_t train_threads[NUM_TRAINS];
int current_train = 0;  // Variable to keep track of the current train crossing
int trains_crossed = 0; // Counter for trains that have crossed

void* train_thread(void* arg) {
    int train_number = *((int*)arg);
    // Simulate loading time
    usleep(LOADING_TIME * 1000000);  // Convert to microseconds

    pthread_mutex_lock(&station_mutex);
    printf("Train %d is ready to go\n", train_number);
    pthread_cond_signal(&start_loading_condition);  // Signal that the train is ready

    // Wait until it's time to cross
    while (current_train != train_number) {
        pthread_cond_wait(&crossing_condition, &station_mutex);
    }
    pthread_mutex_unlock(&station_mutex);

    // Simulate crossing time
    usleep(CROSSING_TIME * 1000000);  // Convert to microseconds

    printf("Train %d has crossed the main track\n", train_number);

    // Update the current_train variable and signal all waiting trains
    pthread_mutex_lock(&station_mutex);
    current_train = (current_train + 1) % NUM_TRAINS;  // Move to the next train
    trains_crossed++;
    pthread_cond_broadcast(&crossing_condition);  // Signal all waiting trains
    pthread_mutex_unlock(&station_mutex);

    pthread_exit(NULL);
}

int main() {
    pthread_mutex_lock(&station_mutex);

    // Initialize trains
    for (int i = 0; i < NUM_TRAINS; i++) {
        trains[i].number = i;
    }

    // Create train threads
    for (int i = 0; i < NUM_TRAINS; i++) {
        if (pthread_create(&train_threads[i], NULL, train_thread, (void*)&trains[i].number) != 0) {
            fprintf(stderr, "Error creating thread for Train %d\n", trains[i].number);
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all trains to be ready
    for (int i = 0; i < NUM_TRAINS; i++) {
        pthread_cond_wait(&start_loading_condition, &station_mutex);
    }
    pthread_mutex_unlock(&station_mutex);

    // Wait for all train threads to finish
    while (trains_crossed < NUM_TRAINS) {
        pthread_cond_wait(&crossing_condition, &station_mutex);
    }

    pthread_mutex_unlock(&station_mutex);

    // Clean up
    pthread_mutex_destroy(&station_mutex);
    pthread_cond_destroy(&start_loading_condition);
    pthread_cond_destroy(&crossing_condition);

    return 0;
}

