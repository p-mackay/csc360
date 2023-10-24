#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#define EAST 0 //direction
#define WEST 1 //direction

#define HIGH 1 //priority
#define LOW 0  //priority

#define BILLION 1000000000L

struct timespec start, stop;
double accum;

pthread_mutex_t station_mutex, track_mutex, dispatch_mutex;
pthread_cond_t ready_cv, cross_cv, on_track_cv, dispatch_cv;

typedef struct {
    int number;
    char direction;
    int priority;
    float loadingTime;
    float crossingTime;
    pthread_cond_t *train_cv;
    struct timespec loadFinishTime;
    int orderInFile;
    int readyForDispatch;
} Train;

typedef struct node {
    Train *data;
    int priority;
    struct node *next;
} Node;

Node *westBoundStation = NULL;
Node *eastBoundStation = NULL;

int numTrains = 0;
int builtTrains = 0;
int dispatching = 0;
int on_track = 0;

Node *dispatchQueue = NULL;

int lastTrainDirection = WEST;
int lastTrainPriority;
int lastTrainOrder;
int westDispatched = 0;
int eastDispatched = 0;

void printTime()
{
    if (clock_gettime(CLOCK_REALTIME, &stop) == -1)
    {
        perror("clock gettime");
        exit(EXIT_FAILURE);
    }

    accum = (stop.tv_sec - start.tv_sec) + (stop.tv_nsec - start.tv_nsec) / (double)BILLION;
    int minutes = (int)accum / 60;
    int hours = (int)accum / 3600;
    printf("%02d:%02d:%04.1f ", hours, minutes, accum);
}

void startTimer()
{
    if (clock_gettime(CLOCK_REALTIME, &start) == -1)
    {
        perror("clock gettime");
        exit(EXIT_FAILURE);
    }
}

int getNumTrains(FILE *file)
{
    int count = 0;
    char c;
    while ((c = fgetc(file)) != EOF)
    {
        if (c == '\n')
        {
            count++;
        }
    }
    return count;
}

int getPriority(char direction)
{
    if (direction == 'E' || direction == 'W')
    {
        return HIGH;
    }
    else if (direction == 'e' || direction == 'w')
    {
        return LOW;
    }
    else
    {
        return LOW;
    }
}

char *getDirection(char dir)
{
    switch (dir)
    {
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

void push(Node **head, Train *data)
{
    Node *temp = (Node *)malloc(sizeof(Node));
    temp->data = data;
    temp->priority = data->priority;
    temp->next = NULL;

    if (*head == NULL || (*head)->priority < temp->priority)
    {
        temp->next = *head;
        *head = temp;
    }
    else
    {
        Node *start = *head;
        while (start->next != NULL && start->next->priority >= temp->priority)
        {
            start = start->next;
        }

        temp->next = start->next;
        start->next = temp;
    }
}

Train *pop(Node **head)
{
    if (*head == NULL)
    {
        return NULL;
    }
    else
    {
        Node *temp = *head;
        *head = (*head)->next;
        Train *poppedTrain = temp->data;
        free(temp);
        return poppedTrain;
    }
}

int isEmpty(Node **head)
{
    return (*head) == NULL;
}

void createTrains(Train *trains, char *f, pthread_cond_t *train_conditions)
{
    for (int i = 0; i < numTrains; i++)
    {
        pthread_cond_init(&train_conditions[i], NULL);
    }

    char direction[3], loading_time[3], crossing_time[3];
    int train_number = 0;
    FILE *fp = fopen(f, "r");
    while (fscanf(fp, "%s %s %s", direction, loading_time, crossing_time) != EOF)
    {
        trains[train_number].number = train_number;
        trains[train_number].direction = direction[0];
        trains[train_number].priority = getPriority(direction[0]);
        trains[train_number].loadingTime = atoi(loading_time);
        trains[train_number].crossingTime = atoi(crossing_time);
        trains[train_number].train_cv = &train_conditions[train_number];
        trains[train_number].readyForDispatch = 0;
        trains[train_number].orderInFile = train_number;  // Assigning orderInFile
        train_number++;
        builtTrains++;
    }
    fclose(fp);
}

void dispatchTrain(Train *train)
{
    train->readyForDispatch = 0;
}

void *dispatcher_thread_routine(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&dispatch_mutex);

        while (isEmpty(&dispatchQueue))
        {
            pthread_cond_wait(&dispatch_cv, &dispatch_mutex);
        }

        Train *train = pop(&dispatchQueue);
        pthread_mutex_unlock(&dispatch_mutex);

        dispatchTrain(train);
    }
}

void updateLastTrainDirection(Train *train)
{
    lastTrainDirection = (train->direction == 'E' || train->direction == 'e') ? EAST : WEST;
    lastTrainPriority = train->priority;
    lastTrainOrder = train->orderInFile;
}

int compareLoadFinishTimes(const void *a, const void *b)
{
    const Train *trainA = (const Train *)a;
    const Train *trainB = (const Train *)b;

    if (trainA->loadFinishTime.tv_sec < trainB->loadFinishTime.tv_sec)
    {
        return -1;
    }
    else if (trainA->loadFinishTime.tv_sec > trainB->loadFinishTime.tv_sec)
    {
        return 1;
    }
    else
    {
        if (trainA->loadFinishTime.tv_nsec < trainB->loadFinishTime.tv_nsec)
        {
            return -1;
        }
        else if (trainA->loadFinishTime.tv_nsec > trainB->loadFinishTime.tv_nsec)
        {
            return 1;
        }
        else
        {
            return trainA->priority - trainB->priority;
        }
    }
}

int compareOrderInFile(const void *a, const void *b)
{
    const Train *trainA = (const Train *)a;
    const Train *trainB = (const Train *)b;

    if (trainA->orderInFile < trainB->orderInFile)
    {
        return -1;
    }
    else if (trainA->orderInFile > trainB->orderInFile)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int canDispatch(Train *train)
{
    if (train->readyForDispatch)
    {
        if (train->priority > lastTrainPriority)
        {
            return 1;
        }
        else if (train->priority == lastTrainPriority && train->orderInFile > lastTrainOrder)
        {
            return 1;
        }
        else if ((train->direction == lastTrainDirection ||
            (tolower(train->direction) == 'e' && tolower(lastTrainDirection) == 'e') ||
            (tolower(train->direction) == 'w' && tolower(lastTrainDirection) == 'w')) &&
            train->priority > lastTrainPriority)
        {
            return 1;
        }
        else if (train->direction == lastTrainDirection)
        {
            return 0;
        }
        else if (train->direction == 'E' || train->direction == 'e')
        {
            return !eastDispatched;
        }
        else
        {
            return !westDispatched;
        }
    }
    return 0;
}

void tryDispatchTrain(Train *trains)
{
    printf("Trying to dispatch a train...\n");
    fflush(stdout);

    // Sort trains by load finish time and priority
    qsort(trains, numTrains, sizeof(Train), compareLoadFinishTimes);

    for (int i = 0; i < numTrains; i++)
    {
        if (canDispatch(&trains[i]))
        {
            // Check if the train has the same priority and order in file as the last train
            if (trains[i].priority == lastTrainPriority && trains[i].orderInFile == lastTrainOrder)
            {
                continue;  // Skip this train if it has the same priority and order as the last one
            }

            dispatchTrain(&trains[i]);
            updateLastTrainDirection(&trains[i]);

            if (trains[i].direction == 'E' || trains[i].direction == 'e')
            {
                eastDispatched = 1;
                westDispatched = 0;
            }
            else
            {
                westDispatched = 1;
                eastDispatched = 0;
            }

            return;
        }
    }
}


void *train_thread_routine(void *arg)
{
    Train *train = (Train *)arg;
    unsigned int loadTime = (train->loadingTime) * 100000;
    while (builtTrains < numTrains)
    {
        // Wait until the train is in a ready state (loading completed)
        pthread_mutex_lock(&station_mutex);
        while (!train->readyForDispatch)
        {
            pthread_cond_wait(train->train_cv, &station_mutex);
        }
        pthread_mutex_unlock(&station_mutex);
    }

    usleep(loadTime);
    printTime();
    printf("Train %d is ready to go %s.\n", train->number, getDirection(train->direction));


    pthread_mutex_lock(&station_mutex);
    train->readyForDispatch = 1;
        if (train->direction == 'E' || train->direction == 'e') {
        push(&eastBoundStation, train);
    } else {
        push(&westBoundStation, train);
    }

    pthread_mutex_unlock(&station_mutex);
    pthread_cond_broadcast(&ready_cv);

    pthread_mutex_lock(&dispatch_mutex);
    push(&dispatchQueue, train);
    pthread_cond_signal(&dispatch_cv);
    pthread_mutex_unlock(&dispatch_mutex);

    pthread_mutex_lock(&track_mutex);
    while (on_track)
    {
        pthread_cond_wait(&on_track_cv, &track_mutex);
    }

    on_track = 1;
    pthread_mutex_unlock(&track_mutex);

    printTime();
    printf("Train %d is ON the main track going %s\n", train->number, getDirection(train->direction));
    unsigned int crossTime = (train->crossingTime) * 100000;
    usleep(crossTime);

    pthread_mutex_lock(&track_mutex);
    on_track = 0;
    printTime();
    printf("Train %d is OFF the main track going %s\n", train->number, getDirection(train->direction));
    pthread_cond_signal(&on_track_cv);
    pthread_cond_signal(&cross_cv);
    pthread_mutex_unlock(&track_mutex);
    clock_gettime(CLOCK_REALTIME, &(train->loadFinishTime));

    pthread_exit(0);
}

int main(int argc, char *argv[])
{
    FILE *file;

    if (argc > 1)
    {
        file = fopen(argv[1], "r");
        if (file == NULL)
        {
            perror("Error opening file");
            return -1;
        }
        numTrains = getNumTrains(file);
        fclose(file);
    }
    else
    {
        printf("Usage: %s <filename>\n", argv[0]);
        return -1;
    }

    pthread_mutex_init(&station_mutex, NULL);
    pthread_mutex_init(&track_mutex, NULL);
    pthread_mutex_init(&dispatch_mutex, NULL);
    pthread_cond_init(&ready_cv, NULL);
    pthread_cond_init(&on_track_cv, NULL);
    pthread_cond_init(&cross_cv, NULL);
    pthread_cond_init(&dispatch_cv, NULL);

    pthread_t train_threads[numTrains];
    pthread_t dispatcher_thread;
    pthread_cond_t train_conditions[numTrains];

    Train *trains = malloc(numTrains * sizeof(*trains));
    createTrains(trains, argv[1], train_conditions);

    startTimer();

    pthread_create(&dispatcher_thread, NULL, dispatcher_thread_routine, NULL);

    for (int i = 0; i < numTrains; i++)
    {
        pthread_create(&train_threads[i], NULL, train_thread_routine, (void *)&trains[i]);
    }

    for (int i = 0; i < numTrains; i++)
    {
        if (pthread_join(train_threads[i], NULL) != 0)
        {
            perror("Failed to join thread");
        }
    }


    printf("Before tryDispatchTrain...\n");
    for (int i = 0; i < numTrains; i++)
    {
        printf("Train %d: direction=%c, priority=%d, orderInFile=%d, readyForDispatch=%d\n",
               trains[i].number, trains[i].direction, trains[i].priority, trains[i].orderInFile, trains[i].readyForDispatch);
    }
    printf("East Dispatched: %d, West Dispatched: %d\n", eastDispatched, westDispatched);
    fflush(stdout);

    tryDispatchTrain(trains);

    // Print the details of each train after tryDispatchTrain
    printf("After tryDispatchTrain...\n");
    for (int i = 0; i < numTrains; i++)
    {
        printf("Train %d: direction=%c, priority=%d, orderInFile=%d, readyForDispatch=%d\n",
               trains[i].number, trains[i].direction, trains[i].priority, trains[i].orderInFile, trains[i].readyForDispatch);
    }
    printf("East Dispatched: %d, West Dispatched: %d\n", eastDispatched, westDispatched);
    fflush(stdout);


    // Signal the dispatcher to exit
    pthread_cancel(dispatcher_thread);
    pthread_mutex_destroy(&station_mutex);
    pthread_mutex_destroy(&track_mutex);
    pthread_mutex_destroy(&dispatch_mutex);
    pthread_cond_destroy(&ready_cv);
    pthread_cond_destroy(&cross_cv);
    pthread_cond_destroy(&dispatch_cv);

    return 0;
}

