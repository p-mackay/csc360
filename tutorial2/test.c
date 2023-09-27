#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>

#define BUFFER_LEN 1024

typedef struct bg_pro{
    pid_t pid;
    char command[1024];
    struct bg_pro* next;
}bg_pro;

void deallocate(bg_pro** root){
    bg_pro* curr = *root;
    while (curr->next != NULL){
        bg_pro* aux = curr;
        curr = curr->next;
        free(aux);
    }
    *root = NULL;
}

void insert_end(bg_pro** root, pid_t value, char* cmd){
    bg_pro* new_node = malloc(sizeof(bg_pro));
    if (new_node == NULL){
        exit(1);
    }
    new_node->next = NULL;
    new_node->pid = value;
    strcpy(new_node->command, cmd);

    if (*root == NULL){
        *root = new_node;
        return;
    }

    bg_pro* curr = *root;
    while (curr->next != NULL){
        curr = curr->next;
    }
    curr->next = new_node;

}
int main(){
    bg_pro* root = NULL;
    char line[BUFFER_LEN];      /*store what is entered from stdin*/
    char cp_line[BUFFER_LEN];      /*store what is entered from stdin*/
    char* argv[100];            /*stdin args*/
    int argc;                   /*number of args*/
    int bailout = 0;            /*condition to exit SSI*/
    char buff[PATH_MAX];
    char* cwd = getcwd(buff, PATH_MAX);
    char* prompt;
    char* home_dir;
    home_dir = getenv("HOME");  /*get users home directory*/


    while(!bailout){

        printf("%s", cwd);                    //print shell prompt

        if(!fgets(line, BUFFER_LEN, stdin)){  //get command and put it in line
            break;                                //if user hits CTRL+D break
        }
        strcpy(cp_line, line);
        //tokenize input
        //----------------------------
        argv[0]=strtok(line, " \n");    //"\n" includes space & new line
        int i=0;
        while(argv[i]!=NULL){
            argv[i+1]=strtok(NULL," \n");
            i++;
        }
        argv[i]=NULL;                     //set last value to NULL for execvp

        /*
        argc=i;                           //get arg count
        for(i=0; i<argc; i++){
            printf("%s\n", argv[i]);      //print command/args
        }
        */

        if (strcmp(argv[0], "cd") == 0){
            /*Change directory then update current working directory*/
            if(argv[1]==NULL || strcmp(argv[1],"~")==0){
                chdir(home_dir);
                cwd = getcwd(buff,PATH_MAX);
            }else{
                chdir(argv[1]);
                cwd = getcwd(buff,PATH_MAX);
            }
        }else if(strcmp(argv[0], "bg") == 0){
            /*Execute a background process. Add it a linkedlist*/
            pid_t pid = fork();
            if(pid==0){               //Child
                //TODO while argv != null (might be more than 2 args)
                int i = 0;
                while(argv[i]!=NULL){
                    argv[i] = argv[i+1];
                    i++;
                }
                argv[i]=NULL;                     //set last value to NULL for execvp
                execvp(argv[0],argv);
                return 0;
            }else{                    //Parent
                memmove(cp_line, cp_line+3, strlen(cp_line));
                wait(NULL);
                insert_end(&root, pid, cp_line);
                printf("Child exited\n");
            }
        }else{
            /*execute arbitrary commands i.e ls, ./file*/
            pid_t pid = fork();
            if(pid==0){               //Child
                execvp(argv[0],argv);
                return 0;
            }else{                    //Parent
                wait(NULL);
                printf("Child exited\n");
            }
        }

        if(!strcmp(line, "exit")){            //check if command is exit
            bailout = 1;
        }
    }
    for (bg_pro* curr = root; curr != NULL; curr = curr->next){
        printf("pid: %d ", curr->pid);
        printf("command: %s terminated\n", curr->command);
    }
    deallocate(&root);
    /*
    */
} 
