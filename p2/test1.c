#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_TRAINS 5

typedef struct {
    int number;
    char direction;
    int loadingTime;
    int crossingTime;
} Train;

void* trainThread(void* arg);

void printTime() {
    struct timespec time1;
    clock_gettime(CLOCK_REALTIME, &time1);
    printf("%02ld:%02ld:%02ld ", (time1.tv_sec / 3600) % 24, (time1.tv_sec / 60) % 60, time1.tv_sec % 60);
}

int main() {
    pthread_t trainThreads[NUM_TRAINS];
    Train trains[NUM_TRAINS];

    srand(time(NULL)); // Seed for random values

    for (int i = 0; i < NUM_TRAINS; ++i) {
        trains[i].number = i + 1;
        trains[i].direction = (i % 2 == 0) ? 'E' : 'W';
        trains[i].loadingTime = rand() % 5 + 1;
        trains[i].crossingTime = rand() % 5 + 1;
    }

    for (int i = 0; i < NUM_TRAINS; ++i) {
        if (pthread_create(&trainThreads[i], NULL, trainThread, (void*)&trains[i]) != 0) {
            perror("Error creating train thread");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_TRAINS; ++i) {
        if (pthread_join(trainThreads[i], NULL) != 0) {
            perror("Error joining train thread");
            exit(EXIT_FAILURE);
        }
    }

    printf("All trains have finished crossing the main track. Simulation complete.\n");

    return 0;
}

void* trainThread(void* arg) {
    Train* train = (Train*)arg;

    // Simulate loading time
    sleep(train->loadingTime);

    printTime();
    printf("Train %d is ready to go %s\n", train->number, (train->direction == 'E') ? "East" : "West");

    // TODO: Implement synchronization and main track crossing logic

    printTime();
    printf("Train %d is ON the main track going %s\n", train->number, (train->direction == 'E') ? "East" : "West");

    // Simulate crossing time
    sleep(train->crossingTime);

    printTime();
    printf("Train %d is OFF the main track after going %s\n", train->number, (train->direction == 'E') ? "East" : "West");

    pthread_exit(NULL);
}

