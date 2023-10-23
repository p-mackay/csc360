#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <signal.h>

int main(){
    int i = 0;
    printf("%d\n", i);
    
    if(fork() == 0){
        i+=2;
        printf("%d\n", i);
    }else{
        i+=1;
        printf("%d\n", i);
        wait(NULL);
        return 0;
    }
} 
