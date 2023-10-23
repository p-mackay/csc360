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
pthread_cond_t ready_cv, cross_cv;

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
void push(Node** head, Train* data)  
{  
    Node* start = *head;  
  
    // Create new Node  
    Node* temp = newNode(data);  
  
    // Special Case: The head of list has lesser  
    // priority than new node. So insert new  
    // node before head node and change head node.  
    if (*head == NULL) {
        *head = temp;
        return;
    }

    if ((*head)->priority < temp->priority) {  
  
        // Insert New Node before head  
        temp->next = *head;  
        (*head) = temp;  
    }  
    else {  
  
        // Traverse the list and find a  
        // position to insert new node  
        while (start->next != NULL && start->next->priority >= temp->priority) {  
            start = start->next;  
        }  
        // Either at the ends of the list  
        // or at required position  
        temp->next = start->next;  
        start->next = temp;  
    }  
}  

//==================================
//Global Variables 
Node* westBoundSt;
Node* eastBoundSt;
int numTrains = 0;
int builtTrains = 0;
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
}


void createTrains(Train *trains, char *f, pthread_cond_t *train_conditions) {
    // Initiate the condition variables
    for (int i = 0; i < numTrains; i++) {
        pthread_cond_init(&train_conditions[i], NULL);
    }

    char direction[3], loading_time[3], crossing_time[3];
    int train_number = 0;
    FILE *fp = fopen(f, "r");
    while (fscanf(fp, "%s %s %s", direction, loading_time, crossing_time) != EOF) {
        trains[train_number].number = train_number;
        trains[train_number].direction = direction[0];
        trains[train_number].priority = (direction[0]);
        trains[train_number].loadingTime = atoi(loading_time);
        trains[train_number].crossingTime = atoi(crossing_time);
        trains[train_number].train_cv = &train_conditions[train_number];
        train_number++;
        builtTrains++;
    }
    fclose(fp);
}


char* getDirection(char dir){
    /*given a direction like e or E 
     * returns EAST or WEST*/
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

void* train_thread_routine(void* arg) {
    Train* train = (Train*)arg;
    unsigned int loadTime = (train->loadingTime) * 100000;
    while (builtTrains < numTrains) {}

    usleep(loadTime);
    printTime();
    printf("Train %d is ready to go %s.\n", train->number, &train->direction);
    pthread_mutex_lock(&station_mutex);
    push(&eastBoundSt, train);
    pthread_mutex_unlock(&station_mutex);
    pthread_cond_broadcast(&ready_cv);
    sleep(1);

    pthread_exit(0);

}

void* dispatch_thread_routine(void* arg) {
    while (builtTrains < numTrains) {
        pthread_cond_wait(&ready_cv, &station_mutex); // Wait for a train to be ready
        pthread_mutex_lock(&track_mutex); // Lock the track
        Train* currentTrain = peek(&eastBoundSt); // Assuming eastBoundSt has higher priority
        if (currentTrain != NULL) {
            pop(&eastBoundSt); // Remove the train from the queue
            printTime();
            printf("Train %d is ON the main track going %s.\n", currentTrain->number, &currentTrain->direction);
            usleep(currentTrain->crossingTime * 1000000); // Simulate crossing time
            pthread_mutex_unlock(&track_mutex); // Unlock the track
            pthread_cond_signal(currentTrain->train_cv); // Signal the train that it's done crossing
        }
    }
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






/*
    Train *ptrain = malloc(sizeof(*ptrain));
    Train *ptrain2 = malloc(sizeof(*ptrain));

    ptrain->number = 0;
    ptrain->priority = 0;
    ptrain->direction= 0;
    ptrain->loadingTime = 1;
    ptrain->crossingTime = 1;
    ptrain->train_cv = &ready_cv;

    ptrain2->number = 1;
    ptrain2->priority = 0;
    ptrain->direction= 0;
    ptrain2->loadingTime = 3;
    ptrain2->crossingTime = 1;
    ptrain2->train_cv = &ready_cv;
    //newNode(ptrain);
    //newNode(ptrain2);
    push(&westBoundSt, ptrain);
    push(&westBoundSt, ptrain2);
    printf("priority: %d\n", peekPriority(&westBoundSt));
    //pop(&westBoundSt);
    //}

    startTimer();
    pthread_t th[2];

    //for (int i = 0; i < 2; i++){
        if (pthread_create(&th[0], NULL, &train_thread_routine, (void *)&ptrain) != 0) {
            perror("Failed to create thread");
        }
        if (pthread_create(&th[1], NULL, &train_thread_routine, (void *)&ptrain2) != 0) {
            perror("Failed to create thread");
        }
    //}
    
   */ 
    pthread_mutex_destroy(&station_mutex);
    pthread_mutex_destroy(&track_mutex);
    pthread_cond_destroy(&ready_cv);
    pthread_cond_destroy(&cross_cv);





    return 0;
}

