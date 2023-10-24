#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define EAST 0
#define WEST 1

#define HIGH 1
#define LOW 0

#define BILLION 1000000000L; 
struct timespec start, stop;
double accum;

pthread_mutex_t station_mutex, track_mutex;
pthread_cond_t ready_cv, cross_cv, on_track_cv;

// Train structure
typedef struct {
    int number;
    char direction;
    int priority; 
    float loadingTime;
    float crossingTime;
    pthread_cond_t *train_cv;
} Train;

// Node structure for the linked list
typedef struct Node {
    Train* data;
    int priority;
    struct Node* next;
} Node;

// Priority Queue structure
typedef struct {
    Node* front;
} PriorityQueue;

// Function to create a new node
Node* newNode(Train* data) {
    Node* temp = (Node*)malloc(sizeof(Node));
    temp->data = data;
    temp->priority = data->priority;
    temp->next = NULL;
    return temp;
}

// Function to create a new priority queue
PriorityQueue* createPriorityQueue() {
    PriorityQueue* queue = (PriorityQueue*)malloc(sizeof(PriorityQueue));
    queue->front = NULL;
    return queue;
}

// Function to enqueue a new element into the priority queue
void enqueue(PriorityQueue* queue, Train* data) {
    // Create new Node
    Node* temp = newNode(data);

    // Special Case: The queue is empty or the new node has higher priority than the front.
    // So insert the new node before the front node and change the front node.
    if (queue->front == NULL || data->priority > queue->front->priority) {
        temp->next = queue->front;
        queue->front = temp;
    } else {
        // Traverse the list and find a position to insert the new node.
        Node* start = queue->front;
        while (start->next != NULL && start->next->priority >= temp->priority) {
            start = start->next;
        }

        // Either at the ends of the list or at the required position.
        temp->next = start->next;
        start->next = temp;
    }
}

// Function to dequeue the element with the highest priority
Train* dequeue(PriorityQueue* queue) {
    if (queue->front == NULL) {
        printf("Priority Queue is empty.\n");
        return NULL; // Return a sentinel value indicating an empty queue
    }

    Node* temp = queue->front;
    Train* data = temp->data;
    queue->front = temp->next;
    free(temp);

    return data;
}

// Function to check if the priority queue is empty
int isEmpty(PriorityQueue* queue) {
    return queue->front == NULL;
}

// Function to peek at the front element of the priority queue
Train* peek(PriorityQueue* queue) {
    if (queue->front == NULL) {
        printf("Priority Queue is empty.\n");
        return NULL;
    }
    return queue->front->data;
}

// Function to peek at the priority of the front element of the priority queue
int peekPriority(PriorityQueue* queue) {
    if (queue->front == NULL) {
        printf("Priority Queue is empty.\n");
        return -1; // Return a sentinel value indicating an empty queue
    }
    return queue->front->priority;
}

// Function to free the memory allocated for the priority queue
void destroyQueue(PriorityQueue* queue) {
    while (queue->front != NULL) {
        Node* temp = queue->front;
        queue->front = temp->next;
        free(temp);
    }
    free(queue);
}

//----------------------------------------------------------------
//
//==================================
//Global Variables 
PriorityQueue* westBoundStation;
PriorityQueue* eastBoundStation;
int numTrains = 0;
int builtTrains = 0;
int dispatching = 0;
int on_track = 0;
//==================================

int getNumTrains(FILE *file){
    int count = 0;
    char c;
    // Iterate through each character in the file
    while ((c = fgetc(file)) != EOF) {
        if (c == '\n') {
            count++;  // Increment the count for each newline character
        }
    }
    return count;
}

int getPriority(char direction){
    /*Given a direction returns either HIGH or LOW priority*/
    if (direction == 'E' || direction == 'e') {
        return HIGH;
    } else if (direction == 'W' || direction == 'w') {
        return LOW;
    } else {
        // Handle unknown direction
        return LOW;
    }
}


void createTrains(Train *trains, char *f, pthread_cond_t *train_conditions) {

    for (int i = 0; i < numTrains; i++) {
        pthread_cond_init(&train_conditions[i], NULL);
    }

    char direction[3], loading_time[3], crossing_time[3];
    int train_number = 0;
    FILE *fp = fopen(f, "r");
    while (fscanf(fp, "%s %s %s", direction, loading_time, crossing_time) != EOF) {
        trains[train_number].number = train_number;
        trains[train_number].direction = direction[0];
        trains[train_number].priority = getPriority(direction[0]);
        trains[train_number].loadingTime = atoi(loading_time);
        trains[train_number].crossingTime = atoi(crossing_time);
        trains[train_number].train_cv = &train_conditions[train_number];
        train_number++;
        builtTrains++;
    }
    fclose(fp);
}

/*
int nextTrain(int previous) {
    if (!isEmpty(&stationEastHead) && !isEmpty(&stationWestHead)) {
        
        // Below 2 conditionals ensure trains in 1 direction do not exceed 3 in a row
        if (numEast == 3) {
            numEast = 0;
            return WEST;
        }

        if (numWest == 3) {
            numWest = 0;
            return EAST;
        }
        
        if (peekPriority(&stationEastHead) > peekPriority(&stationWestHead)) { 
            return EAST;
        } else if (peekPriority(&stationEastHead) < peekPriority(&stationWestHead)) {
            return WEST;
        } else {
            if (previous == 1 || previous == -1) return EAST;
            else return WEST;
        }
        
    } else if (!isEmpty(&stationEastHead) && isEmpty(&stationWestHead)) {
        return EAST;
    } else if (isEmpty(&stationEastHead) && !isEmpty(&stationWestHead)) {
        return WEST;
    }
    return -10;
}
*/

char* getDirection(char dir){
    /*given a direction like e or E EAST 0
     * returns EAST or WEST         WEST 1*/
    switch (dir) {
        case 'e':
        case 'E':
            return "East";
        case 'w':
        case 'W':
            return "West";
        default:
            return "UNKNOWN";
    }

}

void printTime(){
        if (clock_gettime(CLOCK_REALTIME, &stop) == -1) {
        perror("clock gettime");
        exit(EXIT_FAILURE);
    }

    accum = (stop.tv_sec - start.tv_sec) + (stop.tv_nsec - start.tv_nsec) / (double)BILLION;
    int minutes = (int)accum/60;
    int hours = (int)accum/3600;
    printf("%02d:%02d:%04.1f ", hours, minutes, accum);
}

void startTimer(){
       if(clock_gettime(CLOCK_REALTIME, &start) == -1) {
        perror("clock gettime");
        exit(EXIT_FAILURE);
    }
}

void *train_thread_routine(void *arg) {
    Train *train = (Train *)arg;
    unsigned int loadTime = (train->loadingTime) * 100000;
    while (builtTrains < numTrains) {}

    usleep(loadTime);
    printTime();
    printf("Train %d is ready to go %s.\n", train->number, getDirection(train->direction));

    pthread_mutex_lock(&station_mutex);

    // Check if there is a train on the track and compare priorities
    if (on_track) {
        PriorityQueue* currentQueue = train->direction == 'E' ? eastBoundStation : westBoundStation;

        // Check if the priority queue is not empty
        if (!isEmpty(currentQueue)) {
            Train* onTrackTrain = peek(currentQueue);
            if (train->priority > onTrackTrain->priority) {
                // High-priority train proceeds immediately
                enqueue(currentQueue, train);
            } else {
                // Low-priority train waits in the priority queue
                enqueue(currentQueue, train);
                pthread_mutex_unlock(&station_mutex);
                pthread_cond_broadcast(&ready_cv);
                pthread_mutex_lock(&track_mutex);
                while (on_track) {
                    pthread_cond_wait(&on_track_cv, &track_mutex);
                }
                on_track = 1;  // Mark the track as occupied
                pthread_mutex_unlock(&track_mutex);
            }
        } else {
            // If the priority queue is empty, the current train proceeds immediately
            enqueue(currentQueue, train);
            pthread_mutex_unlock(&station_mutex);
        }
    } else {
        // If no train on track, high-priority train proceeds immediately
        on_track = 1;
        pthread_mutex_unlock(&station_mutex);
    }

    printTime();
    printf("Train %d is ON the main track going %s\n", train->number, getDirection(train->direction));
    unsigned int crossTime = (train->crossingTime) * 100000;
    usleep(crossTime);

    pthread_mutex_lock(&track_mutex);
    on_track = 0;  // Mark the track as unoccupied
    printTime();
    printf("Train %d is OFF the main track going %s\n", train->number, getDirection(train->direction));
    pthread_cond_signal(&on_track_cv);  // Signal that the track is available

    // If there are waiting trains in the priority queue, dequeue and let the next one proceed
    PriorityQueue* currentQueue = train->direction == 'E' ? eastBoundStation : westBoundStation;
    if (!isEmpty(currentQueue)) {
        Train* nextTrain = dequeue(currentQueue);
        pthread_mutex_unlock(&track_mutex);
        pthread_cond_signal(&on_track_cv);  // Signal that the track is available for the next train
        pthread_cond_signal(&cross_cv);  // Signal that the train has crossed
        pthread_mutex_unlock(&track_mutex);

        pthread_exit(0);
    } else {
        // Log a message if the priority queue is empty (for debugging purposes)
        printf("Priority Queue is empty.\n");
    }

    pthread_cond_signal(&cross_cv);  // Signal that the train has crossed
    pthread_mutex_unlock(&track_mutex);

    pthread_exit(0);
}





int main(int argc, char *argv[]) {
    FILE *file;
    // Check if a filename is provided as a command-line argument
    if (argc > 1) {
        file = fopen(argv[1], "r");
        if (file == NULL) {
            perror("Error opening file");
            return -1;  // Error opening file
        }
        numTrains = getNumTrains(file);
        fclose(file);
    } else {
        printf("Usage: %s <filename>\n", argv[0]);
        return -1;
    }

    pthread_mutex_init(&station_mutex, NULL);
    pthread_mutex_init(&track_mutex, NULL);
    pthread_cond_init(&ready_cv, NULL);
    pthread_cond_init(&on_track_cv, NULL);

    // Initialize priority queues
    eastBoundStation = createPriorityQueue();
    westBoundStation = createPriorityQueue();

    pthread_t train_threads[numTrains];
    pthread_cond_t train_conditions[numTrains];

    Train *trains = malloc(numTrains * sizeof(*trains));
    createTrains(trains, argv[1], train_conditions);

    startTimer();

    for (int i = 0; i < numTrains; i++) {
        pthread_create(&train_threads[i], NULL, train_thread_routine, (void *) &trains[i]);
    }
    for (int i = 0; i < numTrains; i++) {
        if (pthread_join(train_threads[i], NULL) != 0) {
            perror("Failed to join thread");
        }
    }

    // Destroy priority queues
    destroyQueue(eastBoundStation);
    destroyQueue(westBoundStation);

    pthread_mutex_destroy(&station_mutex);
    pthread_mutex_destroy(&track_mutex);
    pthread_cond_destroy(&ready_cv);
    pthread_cond_destroy(&cross_cv);

    return 0;
}

