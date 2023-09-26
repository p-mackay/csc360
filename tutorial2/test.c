#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <sys/wait.h>


#define BUFFER_LEN 1024

int main(){
    char line[BUFFER_LEN];  //get command line
    char* argv[100];        //user command
    int argc;               //arg count
    int bailout = 0;
    char buff[PATH_MAX];
    char* cwd = getcwd(buff, PATH_MAX);
    char* prompt;
    char* home_dir;
    home_dir = getenv("HOME");


    while(!bailout){

        printf("%s", cwd);                    //print shell prompt

        if(!fgets(line, BUFFER_LEN, stdin)){  //get command and put it in line
            break;                                //if user hits CTRL+D break
        }
        //tokenize input
        //----------------------------
        argv[0]=strtok(line, " \n"); //"\n" includes space & new line
        int i=0;
        while(argv[i]!=NULL){
            argv[i+1]=strtok(NULL," \n");
            i++;
        }
        argv[i]=NULL;                     //set last value to NULL for execvp

        argc=i;                           //get arg count
        for(i=0; i<argc; i++){
            printf("%s\n", argv[i]);      //print command/args
        }
        /*
        strcpy(progpath, path);           //copy /bin/ to file path
        strcat(progpath, argv[0]);            //add program to path

        for(i=0; i<strlen(progpath); i++){    //delete newline
            if(progpath[i]=='\n'){      
                progpath[i]='\0';
            }
        }
        */



        if (strcmp(argv[0], "cd") == 0){
            if(argv[1]==NULL || strcmp(argv[1],"~")==0){
                chdir(home_dir);
                cwd = getcwd(buff,PATH_MAX);
            }else{
                chdir(argv[1]);
                cwd = getcwd(buff,PATH_MAX);
            }
        }
        int pid= fork();              //fork child
        if(pid==0){               //Child
            if(strcmp(argv[0], "ls") == 0){
                execvp(argv[0],argv);
                return 0;
            }else{return 0;}

        }else{                    //Parent
            wait(NULL);
            printf("Child exited\n");
        }
        if(!strcmp(line, "exit")){            //check if command is exit
            bailout = 1;
        }
    }
} 
