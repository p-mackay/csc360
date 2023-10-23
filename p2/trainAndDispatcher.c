#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t trainReadyCV = PTHREAD_COND_INITIALIZER;
pthread_cond_t trainOnTrackCV = PTHREAD_COND_INITIALIZER;
pthread_cond_t trainFinishedCV = PTHREAD_COND_INITIALIZER;
pthread_cond_t stationSpaceAvailableCV = PTHREAD_COND_INITIALIZER;
pthread_cond_t mainTrackSpaceAvailableCV = PTHREAD_COND_INITIALIZER;

int mainTrackOccupied = 0;  // Flag to track main track occupancy

typedef struct {
    int trainNumber;
} Train;

void *trainThread(void *arg) {
    Train *train = (Train *)arg;

    // Loading process
    pthread_mutex_lock(&mutex);
    printf("Train %d is ready to go\n", train->trainNumber);
    pthread_cond_signal(&trainReadyCV);
    pthread_mutex_unlock(&mutex);

    // Wait for station space
    pthread_mutex_lock(&mutex);
    while (/* condition */1) {
        pthread_cond_wait(&stationSpaceAvailableCV, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    // Cross the main track
    pthread_mutex_lock(&mutex);
    printf("Train %d is ON the main track\n", train->trainNumber);
    pthread_cond_signal(&trainOnTrackCV);
    pthread_mutex_unlock(&mutex);

    // Wait for main track space
    pthread_mutex_lock(&mutex);
    while (/* condition */1) {
        pthread_cond_wait(&mainTrackSpaceAvailableCV, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    // Post-crossing process
    pthread_mutex_lock(&mutex);
    printf("Train %d is OFF the main track\n", train->trainNumber);
    pthread_cond_signal(&trainFinishedCV);
    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
}

int main() {
    pthread_t train1, train2;

    // Create trains
    Train t1 = {1};
    Train t2 = {2};

    // Create threads for the trains
    pthread_create(&train1, NULL, trainThread, (void *)&t1);
    pthread_create(&train2, NULL, trainThread, (void *)&t2);

    // Dispatcher functionality in main
    while (/* condition */1) {
        pthread_mutex_lock(&mutex);

        // Check if the main track is occupied
        while (mainTrackOccupied) {
            pthread_cond_wait(&mainTrackSpaceAvailableCV, &mutex);
        }

        // Dispatch a train to the main track
        mainTrackOccupied = 1;  // Set main track as occupied
        pthread_cond_broadcast(&trainOnTrackCV);

        pthread_mutex_unlock(&mutex);

        // Simulate train crossing time
        usleep(/* train crossing time */1);

        pthread_mutex_lock(&mutex);

        // Clear main track occupancy
        mainTrackOccupied = 0;
        pthread_cond_broadcast(&mainTrackSpaceAvailableCV);

        pthread_mutex_unlock(&mutex);
    }

    // Wait for trains to finish
    pthread_join(train1, NULL);
    pthread_join(train2, NULL);

    return 0;
}

