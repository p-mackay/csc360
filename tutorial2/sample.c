#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <sys/wait.h>

#define BUFFER_LEN 1024


int
main()
{ 
    //get current working directory getcwd
    char* cwd;
    char* login;
    char buff[PATH_MAX];
    cwd = getcwd(buff,PATH_MAX);
    char line[BUFFER_LEN];

    //getlogin
    login = getlogin();
    char* prompt = cwd;
    char* args[3];
    args[2] = NULL;
    int exec1;


    int bailout = 0;
    while (!bailout) {
        printf("%s", cwd);
        fgets(line, BUFFER_LEN, stdin);

        //tokenize input
        //----------------------------
        args[0]=strtok(line, " \n"); //"\n" includes space & new line
        int i=0;
        while(args[i]!=NULL){
            args[i+1]=strtok(NULL," \n");
            i++;
        }
        args[i]=NULL;
        //----------------------------
        if (strcmp(args[0], "ls") == 0){
        int p = fork();
            if (p == 0){
                printf("%s", cwd);
                printf("hello from child\n");
                printf("%d\n", getpid());
                exec1 = execvp(args[0],args);
                return 0;
            }else{
                wait(NULL);
                printf("hello from main\n");
                printf("%d\n", getpid());
            }
        }else if(strcmp(args[0], "cd") == 0){
            chdir(args[1]);
            cwd = getcwd(buff,PATH_MAX);
            printf("ARGS[1]: %s", args[1]);
            
        }


        /* Note that readline strips away the final \n */
        //Tokenize string entered by user
        //----------------------------
        /*
        int p = fork();
        if(p == 0){
            printf("hello from child\n");
            exec1 = execvp(args[0],args);
            printf("%d\n", exec1);
            printf("%d\n", getpid());
            return 0;
        }
        if(p != 0){
            int pid1 = waitpid(p, NULL, WNOHANG);
            printf("hello from main\n");
        }
        */
        //----------------------------
        //Execute the commands entered
        //----------------------------
        //----------------------------
        if (!strcmp(line, "exit")) {
            bailout = 1;
        } else {
            printf("\nYou said: %s\n\n", line);
        }
    }
    printf("Bye Bye\n");
}
