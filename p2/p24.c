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
pthread_cond_t ready_cv, off_track_cv;

typedef struct {
    int number;
    char direction;
    int priority; 
    float loadingTime;
    float crossingTime;
    pthread_cond_t *train_cv;
} Train;


/* Skeleton code from https: www.geeksforgeeks.org/priority-queue-using-linked-list*/
/* Queue code VVV*/
/*----------------------------------------------------------------*/
/* Node  */
typedef struct node {  
    Train* data;  
    int priority;  
    struct node* next;  

} Node;  


/* Function to Create A New Node  */
Node* newNode(Train* data){
    Node* temp = (Node*)malloc(sizeof(Node));  
    temp->data = data;  
    temp->priority = data->priority;  
    temp->next = NULL;  

    return temp;  
}  


/* Return the value at head  */
Train* peek(Node** head){
    return (*head)->data;  
}  

/* Return the trains priority at head  */
int peekPriority(Node** head){
    return (*head)->priority;  
}  


/* Removes the element with the  */
/* highest priority from the list  */
void pop(Node** head)  
{  
    if (*head == NULL){
        return;
    }else{
        Node* temp = *head;  
        (*head) = (*head)->next;  
        free(temp);  
    }
}  

/* Function to push according to priority, load time, and train number.
 * Further the purpose here is to solve Rule 3, and 4a*/

/* Compare function to decide whether 'temp' should come before 'node' */
int shouldInsertBefore(Node* node, Node* temp) {
    if (node->priority < temp->priority) return 1;
    if (node->priority > temp->priority) return 0;

    if (node->data->loadingTime > temp->data->loadingTime) return 1;
    if (node->data->loadingTime < temp->data->loadingTime) return 0;

    if (node->data->number > temp->data->number) return 1;
    return 0;
}

void push(Node** head, Train* data) {
    /* Create new Node */
    Node* temp = newNode(data);


    /* Check for insertion at the beginning */
    if (*head == NULL || shouldInsertBefore(*head, temp)) {
        temp->next = *head;
        *head = temp;
        return;
    }

    /* Find position for new node */
    Node* start = *head;
    Node* prev = NULL;
    while (start != NULL && !shouldInsertBefore(start, temp)) {
        prev = start;
        start = start->next;
    }

    /* Insert the new node */
    temp->next = start;
    prev->next = temp;
}


/*==================================*/
/* Global Variables */
Node* westBound;    /* Queue to store west bound trains*/
Node* eastBound;    /* Queue to store east bound trains*/
int numTrains = 0;  /* Stores the number of trains*/
int trainCount = 0;
int on_track = 0;           /* Variable to indicate that a train is on the track*/
int consecutive_trains = 0; /* Stores number of consecutive trains in one direction*/
int last_direction = WEST;  /* Stores direction of last train*/
/*==================================*/


/* Function to check if a queue is empty  */
int isEmpty(Node** head)  
{  
    return (*head) == NULL;  
}  
/*----------------------------------------------------------------*/


int getPriority(char direction){
    /*Given a direction returns either HIGH or LOW priority*/
    if (direction == 'E' || direction == 'W') {
        return HIGH;
    } else if (direction == 'e' || direction == 'w') {
        return LOW;
    } else {
        /* Handle unknown direction*/
        return LOW;
    }
}


void fileToTrains(Train *this_train, char *file, pthread_cond_t *train_wait_cv) {

    for (int i = 0; i < numTrains; i++) {
        pthread_cond_init(&train_wait_cv[i], NULL);
    }

    char direction[3], loadTime[3], crossTime[3];
    int trainID = 0;
    FILE *fp = fopen(file, "r");
    while (fscanf(fp, "%s %s %s", direction, loadTime, crossTime) != EOF) {
        this_train[trainID].number = trainID;
        this_train[trainID].priority = getPriority(direction[0]);
        this_train[trainID].direction = direction[0];
        this_train[trainID].loadingTime = atoi(loadTime);
        this_train[trainID].crossingTime = atoi(crossTime);
        this_train[trainID].train_cv = &train_wait_cv[trainID];
        trainID++;
        trainCount++;
    }
    fclose(fp);
}


char* getDirection(char dir){
    /*given a direction e,E or w,W 
     * returns the string East or West*/
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

/* Function to determine the number of trains given a file*/
int getNumTrains(FILE *file){
    int count = 0;
    char c;
    /* Iterate through each character in the file*/
    while ((c = fgetc(file)) != EOF) {
        if (c == '\n') {
            count++;  /* Increment the count for each newline character*/
        }
    }
    return count;
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
    while (trainCount < numTrains) {}

    usleep(loadTime);
    printTime();
    printf("Train %2d is ready to go %s\n", train->number, getDirection(train->direction));

    pthread_mutex_lock(&station_mutex);
    if (train->direction == 'E' || train->direction == 'e') {
        push(&eastBound, train);
    } else {
        push(&westBound, train);
    }
    pthread_mutex_unlock(&station_mutex);
    pthread_cond_signal(&ready_cv);

    pthread_mutex_lock(&track_mutex);
    pthread_cond_wait(train->train_cv, &track_mutex);
    pthread_mutex_unlock(&track_mutex);


    printTime();
    printf("Train %2d is ON the main track going %s\n", train->number, getDirection(train->direction));
    unsigned int crossTime = (train->crossingTime) * 100000;
    usleep(crossTime);

    pthread_mutex_lock(&track_mutex);
    on_track = 0;  /* Mark the track as unoccupied*/
    printTime();
    printf("Train %2d is OFF the main track after going %s\n", train->number, getDirection(train->direction));
    pthread_cond_signal(&off_track_cv);  /* Signal that the track is available*/
    pthread_mutex_unlock(&track_mutex);
    usleep(1000); /* Small sleep before checking again.*/

    pthread_exit(0);
}

void freeQueue(Node** head) {
    while (!isEmpty(head)) {
        pop(head);
    }
}

int main(int argc, char *argv[]) {
    FILE *file;
    /* Check if a filename is provided as a command-line argument*/
    if (argc > 1) {
        file = fopen(argv[1], "r");
        if (file == NULL) {
            perror("Error opening file");
            return -1;  /* Error opening file*/
        }
        numTrains = getNumTrains(file);
        fclose(file);
    } else {
        printf("Usage: %s <filename>\n", argv[0]);
    }

    pthread_mutex_init(&station_mutex, NULL);
    pthread_mutex_init(&track_mutex, NULL);
    pthread_cond_init(&ready_cv, NULL);
    pthread_cond_init(&off_track_cv, NULL);

    pthread_t train_threads[numTrains];
    pthread_cond_t train_wait_cv[numTrains];

    Train *trains = malloc(numTrains * sizeof(*trains));
    fileToTrains(trains, argv[1], train_wait_cv);

    startTimer();

    for (int i = 0; i < numTrains; i++) {
        pthread_create(&train_threads[i], NULL, train_thread_routine, (void *) &trains[i]);
    }

    /*dispatch */
    /* The purpose of dispatch loop is to solve 4b, 4c opposite directions 
     * and starvation*/

    for (int dispatchedTrains = 0; dispatchedTrains < numTrains; ) {
        pthread_mutex_lock(&station_mutex);

        while(isEmpty(&eastBound) && isEmpty(&westBound)) {
            pthread_cond_wait(&ready_cv, &station_mutex);
        }

        Train *nextTrain = NULL;

        if (!isEmpty(&eastBound) || !isEmpty(&westBound)) {
            if (consecutive_trains >= 3) {
                if (last_direction == EAST && !isEmpty(&westBound)) {
                    nextTrain = peek(&westBound);
                    pop(&westBound);
                    last_direction = WEST;
                    consecutive_trains = 1;
                } else if (last_direction == WEST && !isEmpty(&eastBound)) {
                    nextTrain = peek(&eastBound);
                    pop(&eastBound);
                    last_direction = EAST;
                    consecutive_trains = 1;
                } else {
                    nextTrain = (last_direction == EAST) ? peek(&eastBound) : peek(&westBound);
                    pop((last_direction == EAST) ? &eastBound : &westBound);
                    consecutive_trains++;
                }
            } else {
                if (last_direction == EAST) {
                    if (!isEmpty(&westBound)) {
                        nextTrain = peek(&westBound);
                        pop(&westBound);
                        last_direction = WEST;
                        consecutive_trains = 1;
                    } else if (!isEmpty(&eastBound)) {
                        nextTrain = peek(&eastBound);
                        pop(&eastBound);
                        consecutive_trains++;
                    }
                } else {
                    if (!isEmpty(&eastBound) &&
                        (isEmpty(&westBound) || peekPriority(&eastBound) > peekPriority(&westBound))) {
                        nextTrain = peek(&eastBound);
                        pop(&eastBound);
                        last_direction = EAST;
                        consecutive_trains++;
                    } else if (!isEmpty(&westBound)) {
                        nextTrain = peek(&westBound);
                        pop(&westBound);
                        last_direction = WEST;
                        consecutive_trains++;
                    }
                }
            }

            if(nextTrain){
                pthread_cond_signal(nextTrain->train_cv); /* Signal the highest priority train*/
                pthread_mutex_unlock(&station_mutex);

                pthread_mutex_lock(&track_mutex);
                pthread_cond_wait(&off_track_cv, &track_mutex); /* wait until the chosen train has crossed*/
                pthread_mutex_unlock(&track_mutex);

                dispatchedTrains++;
                printf("consecutive_trains: %d last dir: %d\n", 
                       consecutive_trains, last_direction);
            }

        }else {
            pthread_mutex_unlock(&station_mutex);
            usleep(1000); /* Small sleep before checking again.*/
        }
    }

    for (int i = 0; i < numTrains; i++) {
        if (pthread_join(train_threads[i], NULL) != 0) {
            perror("Failed to join thread");
        }
    }
    pthread_cond_destroy(trains->train_cv);
    free(trains);
    freeQueue(&eastBound);
    freeQueue(&westBound);
    pthread_mutex_destroy(&station_mutex);
    pthread_mutex_destroy(&track_mutex);
    pthread_cond_destroy(&ready_cv);
    pthread_cond_destroy(&off_track_cv);

    return 0;
}

