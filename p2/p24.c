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


// Queue code VVV
//----------------------------------------------------------------
// Node  
typedef struct node {  
    Train* data;  
    int priority;  
    struct node* next;  
  
} Node;  

  
// Function to Create A New Node  
Node* newNode(Train* data){
    Node* temp = (Node*)malloc(sizeof(Node));  
    temp->data = data;  
    temp->priority = data->priority;  
    temp->next = NULL;  
  
    return temp;  
}  

  
// Return the value at head  
Train* peek(Node** head)  
{  
    return (*head)->data;  
}  

int peekPriority(Node** head)  
{  
    return (*head)->priority;  
}  

  
// Removes the element with the  
// highest priority from the list  
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
  
// Function to push according to priority  
void push(Node** head, Train* data) {
    // Create new Node
    Node* temp = newNode(data);

    // Special Case: The head of the list has lesser
    // priority than the new node. So insert the new
    // node before the head node and change the head node.
    if (*head == NULL || (*head)->priority < temp->priority) {
        temp->next = *head;
        *head = temp;
    } else {
        // Traverse the list and find a
        // position to insert the new node.
        Node* start = *head;
        while (start->next != NULL && start->next->priority >= temp->priority) {
            start = start->next;
        }

        // Either at the ends of the list
        // or at the required position.
        temp->next = start->next;
        start->next = temp;
    }
}

//==================================
//Global Variables 
Node* westBound;
Node* eastBound;
int numTrains = 0;
int builtTrains = 0;
int dispatching = 0;
int on_track = 0;
//==================================

  
// Function to check is list is empty  
int isEmpty(Node** head)  
{  
    return (*head) == NULL;  
}  
//----------------------------------------------------------------

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
    if (direction == 'E' || direction == 'W') {
        return HIGH;
    } else if (direction == 'e' || direction == 'w') {
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
    if (train->direction == 'E' || train->direction == 'e') {
        push(&eastBound, train);
        printf("%d -> priority: %d  Station priority %d\n",train->number, train->priority, peekPriority(&eastBound));
    } else {
        push(&westBound, train);
        printf("%d -> priority: %d  Station priority %d\n",train->number, train->priority, peekPriority(&westBound));
    }
    pthread_mutex_unlock(&station_mutex);
   // pthread_cond_broadcast(&ready_cv);

    // Wait for the track to be available
    pthread_mutex_lock(&track_mutex);
    while (on_track) {

        pthread_cond_wait(train->train_cv, &track_mutex);
    }

    on_track = 1;  // Mark the track as occupied
    pthread_mutex_unlock(&track_mutex);

    printTime();
    printf("Train %d is ON the main track going %s\n", train->number, getDirection(train->direction));
    unsigned int crossTime = (train->crossingTime) * 100000;
    usleep(crossTime);

    pthread_mutex_lock(&track_mutex);
    on_track = 0;  // Mark the track as unoccupied
    printTime();
    printf("Train %d is OFF the main track going %s\n", train->number, getDirection(train->direction));
    pthread_cond_signal(&off_track_cv);  // Signal that the track is available
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
    }

    pthread_mutex_init(&station_mutex, NULL);
    pthread_mutex_init(&track_mutex, NULL);
    pthread_cond_init(&ready_cv, NULL);
    pthread_cond_init(&off_track_cv, NULL);

    pthread_t train_threads[numTrains];
    pthread_cond_t train_conditions[numTrains];

    Train *trains = malloc(numTrains * sizeof(*trains));
    createTrains(trains, argv[1], train_conditions);

    startTimer();

    for (int i = 0; i < numTrains; i++) {
        pthread_create(&train_threads[i], NULL, train_thread_routine, (void *) &trains[i]);
    }




    // After starting all the train threads:
for (int dispatchedTrains = 0; dispatchedTrains < numTrains; ) {
    pthread_mutex_lock(&station_mutex);
    
    Train *nextTrain = NULL;
    
    if (!isEmpty(&eastBound) || !isEmpty(&westBound)) {
        if (!isEmpty(&eastBound) && 
           (isEmpty(&westBound) || 
           (peekPriority(&eastBound) > peekPriority(&westBound)))) {
            nextTrain = peek(&eastBound);
            pop(&eastBound);
        } else {
            nextTrain = peek(&westBound);
            pop(&westBound);
        }
        
        pthread_mutex_unlock(&station_mutex);
        pthread_cond_signal(nextTrain->train_cv); // Signal the highest priority train
        pthread_cond_wait(&off_track_cv, &track_mutex); // wait until the chosen train has crossed
        dispatchedTrains++;
    } else {
        pthread_mutex_unlock(&station_mutex);
        usleep(1000); // Small sleep before checking again.
    }
}






    for (int i = 0; i < numTrains; i++) {
        if (pthread_join(train_threads[i], NULL) != 0) {
            perror("Failed to join thread");
        }
    }

    pthread_mutex_destroy(&station_mutex);
    pthread_mutex_destroy(&track_mutex);
    pthread_cond_destroy(&ready_cv);
    pthread_cond_destroy(&off_track_cv);

    return 0;
}

