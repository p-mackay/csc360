#include <stdio.h>
#include <stdlib.h>

// Train structure
typedef struct Train {
    int id;
    int priority;
    struct Train* next;
} Train;

// Train station structure
typedef struct TrainStation {
    Train* head;
} TrainStation;

// Function to initialize a train station
void initTrainStation(TrainStation* station) {
    station->head = NULL;
}

// Function to insert a train into the station based on priority
void insertTrain(TrainStation* station, int trainId, int priority) {
    Train* newTrain = (Train*)malloc(sizeof(Train));
    newTrain->id = trainId;
    newTrain->priority = priority;
    newTrain->next = NULL;

    // If the station is empty or the new train has higher priority than the head
    if (station->head == NULL || priority > station->head->priority) {
        newTrain->next = station->head;
        station->head = newTrain;
    } else {
        // Traverse the list to find the appropriate position
        Train* current = station->head;
        while (current->next != NULL && priority <= current->next->priority) {
            current = current->next;
        }

        newTrain->next = current->next;
        current->next = newTrain;
    }
}

// Function to print the trains in the station
void printTrains(TrainStation* station) {
    Train* current = station->head;
    while (current != NULL) {
        printf("Train %d (Priority: %d)\n", current->id, current->priority);
        current = current->next;
    }
}

// Function to free the memory used by the trains
void freeTrains(TrainStation* station) {
    Train* current = station->head;
    while (current != NULL) {
        Train* temp = current;
        current = current->next;
        free(temp);
    }
    station->head = NULL;
}

int main() {
    TrainStation station;
    initTrainStation(&station);

    // Insert trains with different priorities
    insertTrain(&station, 1, 3);
    insertTrain(&station, 2, 1);
    insertTrain(&station, 3, 2);
    insertTrain(&station, 2, 4);
    insertTrain(&station, 4, 3);
    insertTrain(&station, 6, 5);

    // Print the trains in the station
    printf("Trains in the station:\n");
    printTrains(&station);

    // Free the memory used by the trains
    freeTrains(&station);

    return 0;
}

