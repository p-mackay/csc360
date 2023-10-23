#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t trainReadyCV = PTHREAD_COND_INITIALIZER;
pthread_cond_t trainOnTrackCV = PTHREAD_COND_INITIALIZER;
pthread_cond_t trainFinishedCV = PTHREAD_COND_INITIALIZER;
pthread_cond_t stationSpaceAvailableCV = PTHREAD_COND_INITIALIZER;
pthread_cond_t mainTrackCV = PTHREAD_COND_INITIALIZER;

int mainTrackOccupied = 0; // Flag to track main track occupancy

typedef struct
{
    int trainNumber;
} Train;

void *trainThread(void *arg)
{
    Train *train = (Train *)arg;

    // Loading process
    pthread_mutex_lock(&mutex);
    printf("Train %d is ready to go East\n", train->trainNumber);
    pthread_cond_signal(&trainReadyCV);
    pthread_mutex_unlock(&mutex);

    // Wait for station space
    pthread_mutex_lock(&mutex);
    while (/* condition */)
    {
        pthread_cond_wait(&stationSpaceAvailableCV, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    // Cross the main track
    pthread_mutex_lock(&mutex);
    printf("Train %d is ON the main track going East\n", train->trainNumber);
    pthread_cond_signal(&trainOnTrackCV);
    pthread_mutex_unlock(&mutex);

    // Wait for main track space
    pthread_mutex_lock(&mutex);
    while (/* condition */)
    {
        pthread_cond_wait(&mainTrackCV, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    // Post-crossing process
    pthread_mutex_lock(&mutex);
    printf("Train %d is OFF the main track\n", train->trainNumber);
    pthread_cond_signal(&trainFinishedCV);
    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
}

void *dispatcherThread(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&mutex);

        // Check if the main track is occupied
        while (mainTrackOccupied)
        {
            pthread_cond_wait(&mainTrackCV, &mutex);
        }

        // Dispatch a train to the main track
        mainTrackOccupied = 1; // Set main track as occupied
        pthread_cond_broadcast(&trainOnTrackCV);

        pthread_mutex_unlock(&mutex);

        // Simulate train crossing time
        usleep(/* train crossing time */);

        pthread_mutex_lock(&mutex);

        // Clear main track occupancy
        mainTrackOccupied = 0;
        pthread_cond_broadcast(&mainTrackCV);

        pthread_mutex_unlock(&mutex);

        // Simulate time between trains
        sleep(1);
    }
}

int main()
{
    pthread_t train1, train2, dispatcher;

    // Create dispatcher thread
    pthread_create(&dispatcher, NULL, dispatcherThread, NULL);

    // Create trains
    Train t1 = {1};
    Train t2 = {2};

    // Create threads for the trains
    pthread_create(&train1, NULL, trainThread, (void *)&t1);
    pthread_create(&train2, NULL, trainThread, (void *)&t2);

    // Wait for trains to finish
    pthread_join(train1, NULL);
    pthread_join(train2, NULL);

    // Join dispatcher thread (this won't be reached in this example)
    pthread_join(dispatcher, NULL);

    return 0;
}

