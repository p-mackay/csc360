readme.txt for mts.c
====================

Design Proposal (p2a) VS mts.c (p2b) Implementation:
----------------------------------------------------
Main differences include:
- Uses two priority queue's instead of one.
- Each Train struct contains a condition variable.
- Simplified usage of mutex's and condvars:
    mts.c:
    - Uses one mutex to protect the queue (when pushing or popping).
    - Uses one mutex for protecting the track.
    - Uses one condvar to signal its done loading.
    - Uses one condvar to signal its off the track.
    - Each train is initialized with a condvar so that each train can wait/be signaled to access the main track.
- The push function is responsible for enforcing the priority order from highest to lowest:
    - priority
    - loadingTime (if the loadingTime is less then it has higher priority)
    - number (train number, where it appears in the input file)
- The dispatcher loop is responsible for granting access to the main track in special cases:
    - If three trains have consecutively crossed in the same direction then, if 
      the queue in the opposite direction is not empty it will dispatch one from the 
      opposite direction.
    - It also compares priority and load time in special cases where a tie breaker 
      has to be determined. For example when two trains have the same priority but 
      different load times, the train with lower load time is granted access.

1. Overview:
------------
mts.c simulates a multi-threaded train scheduling system. It deals with a single-track 
railway where trains traveling in the same or opposite directions must use the same section of the track. 
To manage this, the program employs priority queues and multithreading to make 
sure trains can pass without collisions, following specific rules on priorities.

2. Key Features:
----------------
- Priority system based on train direction (E, W) and case (upper-case is higher priority).
- Rules to avoid starvation: if three consecutive trains have crossed in one direction, the next will be from the opposite direction.
- Uses condition variables to synchronize the arrival and departure of trains on the track.

3. Data Structures:
-------------------
- Train Struct: Contains information about a train, such as its number, direction, priority, loading, and crossing times.
- Node: Used to implement a priority queue. Contains a train, its priority, and next node reference.
- Priority Queue: Implemented as a linked list (`Node`) which stores trains based on priority, loading time, and train number.

4. Program Structure:
----------------------
Global Variables:
- `eastBound` and `westBound`: Priority queues for east and west-bound trains.
- `consecutive_trains`: Counts the number of consecutive trains in a direction.
- `last_direction`: Keeps track of the last train direction.

Functions:
- `peek`, `pop`, `push`: Operations for the priority queue.
- `peekPriority`: Returns the head's priority. 
- `peekLoadTime`: Returns the head's loadingTime. 
- `getPriority`: Returns train's priority based on its direction (e,w = LOW = 0)(E,W = HIGH = 1) 
- `fileToTrains`: Reads from a file and initializes train structures.
- `getDirection`: Converts e,E to 'East' and w,W to 'West'
- `getNumTrains`: Counts the number of trains based on the input file.
- `printTime` and `startTimer`: Timing utility functions. Based on lab 5.

Thread Function (`train_thread_routine`):
- Waits its loading time, then signals ready.
- Waits for permission to go on the track.
- After crossing, signals that it's off the track.

Main Function:
- Initializes mutexes and condition variables.
- Creates train threads.
- Dispatch loop: Manages which train goes on the track.
- Waits for all train threads to finish.
- Cleans up.

    *Additional:
    1. shouldInsertBefore Function:
    -------------------------------
    Description:
    Determines the relative order of insertion for two trains in the priority queue based on their attributes.

    Parameters:
    - `new_train`: Train being considered for insertion.
    - `curr_train`: Current train in the priority queue.

    Return:
    - True (1) if `new_train` should precede `curr_train`, false (0) otherwise.

    Function Logic:
    Prioritizes based on the train's priority, then loading time, and finally train number.


    2. push Function:
    -----------------
    Description:
    Inserts a train into the correct position in the priority queue.

    Parameters:
    - `head`: Pointer to the priority queue's head.
    - `new_node`: Train node for insertion.

    Return:
    - None. Modifies the priority queue by inserting `new_node`.

    Function Logic:
    Determines the correct position using `shouldInsertBefore` and inserts `new_node` accordingly.


    3. Dispatcher Loop (main):
    --------------------------
    1. Lock the `station_mutex` to ensure exclusive access to station variables.
    2. If both eastBound and westBound priority queues are empty, the thread waits until there's a train ready to be dispatched.
    3. If 3 consecutive trains have crossed in one direction, it tries to switch the dispatch direction.
    4. If a train hasn't been picked based on the direction-switching logic, it picks the next train based on last dispatched direction, priority, and load time.
    5. The chosen train's condition variable (`train_cv`) is signaled to allow it to proceed.
    6. The dispatcher waits until the chosen train has crossed the track.
    7. Logs the current state with consecutive trains dispatched and the last direction.
    8. If no train is picked, the loop sleeps for a short time before trying again.


5. Input File Format:
---------------------
Each line in the input file represents a train with the following format:
[Direction (E/e/W/w)] [LoadingTime] [CrossingTime]

For example:
E 2 5
w 1 3

Where:
- `E` represents a high-priority train heading East.
- `e` represents a low-priority train heading East.
- `W` represents a high-priority train heading West.
- `w` represents a low-priority train heading West.
- LoadingTime is the time taken for a train to get ready.
- CrossingTime is the time taken for a train to cross the track.



