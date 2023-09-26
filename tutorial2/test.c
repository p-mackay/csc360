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

        if (strcmp(argv[0], "cd") == 0){
            if(argv[1]==NULL || strcmp(argv[1],"~")==0){
                chdir(home_dir);
                cwd = getcwd(buff,PATH_MAX);
            }else{
                chdir(argv[1]);
                cwd = getcwd(buff,PATH_MAX);
            }
        }else if(strcmp(argv[0], "ls") == 0){
            pid_t pid = fork();
            if(pid==0){               //Child
                execvp(argv[0],argv);
                return 0;
            }else{                    //Parent
                wait(NULL);
                printf("Child exited\n");
            }
        }else if(strcmp(argv[0], "bg") == 0){
            pid_t pid = fork();
            if(pid==0){               //Child
                //TODO while argv != null (might be more than 2 args)
                argv[0] = argv[1];
                argv[1] = argv[2];
                argv[2] = NULL; 
                return 0;
            }else{                    //Parent
                wait(NULL);
                insert_end(&root, pid, argv[0]);
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
} 
