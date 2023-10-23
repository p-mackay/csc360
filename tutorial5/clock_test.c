#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
 
int main(int argc,char *argv[])
{
    struct timespec time1 = {0};
 
    clock_gettime(CLOCK_REALTIME, &time1);
    printf("CLOCK_REALTIME: (%lu, %lu)\n", time1.tv_sec, time1.tv_nsec);
 
    clock_gettime(CLOCK_MONOTONIC, &time1);
    printf("CLOCK_MONOTONIC: (%lu, %lu)\n", time1.tv_sec, time1.tv_nsec);
 
    sleep(5);
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time1);
    printf("CLOCK_PROCESS_CPUTIME_ID: (%lu, %lu)\n", time1.tv_sec, time1.tv_nsec);
 
    sleep(5);
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time1);
    printf("CLOCK_THREAD_CPUTIME_ID: (%lu, %lu)\n", time1.tv_sec, time1.tv_nsec);
 
    printf("now time :%lu\n", time(NULL));
    return 0;
}