#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <readline/readline.h>
#include <readline/history.h>


#include <sys/wait.h>


int
main()
{ 
    //get current working directory getcwd
    char* cwd;
    char* login;
    char buff[PATH_MAX];
    cwd = getcwd(buff,PATH_MAX);
    //getlogin

    login = getlogin();



    /* TODO:
     * •loop 
        •print:   (use getcwd, getlogin and 
        gethostname)
        username@hostname: /home/user > 
        •read a line from terminal
        •execute the input line by: 
        •fork
        •execvp*
     * */

    //char* s;
    //strcpy(s, login);
    //strcat(s, ": ");
    //strcat(s, cwd);
    char* prompt = cwd;
    char* args[3];
    int exec1;


    int bailout = 0;
    while (!bailout) {
        char* reply = readline(prompt);

        /* Note that readline strips away the final \n */
        //Tokenize string entered by user
        //----------------------------
        int p = fork();
        if(p == 0){
            printf("hello from child\n");
            args[0]=strtok(reply, " \n"); //"\n" includes space & new line
            int i=0;
            while(args[i]!=NULL){
                args[i+1]=strtok(NULL," \n");
                i++;
            }
            exec1 = execvp(args[0],args);
            printf("%d\n", exec1);
            printf("%d\n", getpid());
            return 0;
        }
        if(p != 0){
            int pid1 = waitpid(p, NULL, WNOHANG);
            printf("hello from main\n");
        }
        //----------------------------
        //Execute the commands entered
        //----------------------------
        //----------------------------
        if (strcmp(reply, "cd") == 0){
            chdir("/home/paul/");
            prompt = "/home/paul/";
        }
        if (!strcmp(reply, "bye")) {
            bailout = 1;
        } else {
            printf("\nYou said: %s\n\n", reply);
        }
        free(reply);
    }
    printf("Bye Bye\n");
}
