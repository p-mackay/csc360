#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#define BUFFER_LEN 1024

/*to store pid, command and address of next into a linked list*/
typedef struct bg_pro{
    pid_t pid;
    char command[1024];
    struct bg_pro* next;
}bg_pro;
/*free the momory used by a linkedlist*/
void deallocate(bg_pro** root){
    bg_pro* curr = *root;
    while (curr->next != NULL){
        bg_pro* aux = curr;
        curr = curr->next;
        free(aux);
    }
    *root = NULL;
}
/*insert a node at the end of a linked list*/
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
    bg_pro* root = NULL;        /*initialize an empty linkedlist*/
    char line[BUFFER_LEN];      /*store what is entered from stdin*/
    char cp_line[BUFFER_LEN];   /*copy stdin then remove the \n to store in linkedlist*/
    char* argv[100];            /*stdin args*/
    int bailout = 0;            /*condition to exit SSI*/
    char buff[PATH_MAX];        /*temp variable to store cwd*/
    char* cwd = getcwd(buff, PATH_MAX);         /*current working directory*/
    char* username = getlogin();                /*username*/
    char hostname[HOST_NAME_MAX + 1];           /*hostname*/
    gethostname(hostname, HOST_NAME_MAX + 1);   /*current working directory*/
    char prompt[PATH_MAX];
    char* home_dir;
    home_dir = getenv("HOME");  /*get users home directory*/


    while(!bailout){

        printf("%s@%s: %s >", username, hostname, cwd); /*prompt*/

        if(!fgets(line, BUFFER_LEN, stdin)){    /*get command*/
            break;                              
        }
        if(strcmp(line, "\n") == 0){            /*if user doesn't enter anything then reset loop*/
            continue;
        }
        strcpy(cp_line, line);
        cp_line[strlen(cp_line) - 1] = '\0';    /*store stdin & get rid of the \n*/
        argv[0]=strtok(line, " \n");            /*Tokenize stdin
                                                "\n" includes space & new line*/
        int i=0;
        while(argv[i]!=NULL){
            argv[i+1]=strtok(NULL," \n");
            i++;
        }
        argv[i]=NULL;                       /*set last value to NULL for execvp*/

        if(!strcmp(line, "exit")){          /*check if command is exit*/
            /*if there is a background process running when program is terminated
             * then deallocate those nodes before closing*/
            if(root != NULL){
                deallocate(&root);          
            }
            exit(0);
        }

        if (strcmp(argv[0], "cd") == 0){
            /*Change directory then update current working directory*/
            if(argv[2] != NULL){    /*check if cd takes more than one argument*/
                printf("bash: cd: too many arguments\n");
                continue;
            }else{
                if(argv[1]==NULL || strcmp(argv[1],"~")==0){
                    chdir(home_dir);
                    cwd = getcwd(buff,PATH_MAX);
                }else{
                    chdir(argv[1]);
                    cwd = getcwd(buff,PATH_MAX);
                }
            }
        }else if(strcmp(argv[0], "bg") == 0){
            /*Execute a background process and add it to the linkedlist if it is valid*/
            pid_t ex_code;
            pid_t pid = fork();
            if(pid==0){                 /*child*/
                /*while argv != null (might be more than 2 args) then shift to the left*/
                int i = 0;
                while(argv[i]!=NULL){
                    argv[i] = argv[i+1];
                    i++;
                }
                argv[i]=NULL;           /*set last value to NULL for execvp*/
                ex_code = execvp(argv[0],argv);
                /*if user enters an invalid command then exit*/
                if (ex_code == -1){
                    printf("\ninvalid command\n");
                    printf("%s@%s: %s >", username, hostname, cwd); /*prompt*/
                    exit(EXIT_FAILURE);
                }else{
                    exit(0);
                }
            }else{                      /*parent*/
                /*if it is a valid background process then add it to the 
                 * end of the linked list*/
                if(ex_code != -1){
                    memmove(cp_line, cp_line+3, strlen(cp_line));
                    insert_end(&root, pid, cp_line);
                }

                if(root != NULL){
                    /*after each iteration check if a child has terminated,
                     * if a child has terminated then remove it from the 
                     * linked list and print out which child has terminated.*/
                    pid_t ter = waitpid(0, NULL, WNOHANG);
                    while(ter > 0){
                        if(root->pid == ter){
                            printf("%d:   %s has terminated\n", root->pid, root->command);
                            bg_pro* temp = root;
                            root = root->next;
                            free(temp);
                        }else{
                            bg_pro* curr = root;
                            while (curr->next != NULL){
                                if(curr->next->pid == ter){
                                    printf("%d:   %s has terminated\n", curr->next->pid, curr->next->command);
                                    bg_pro* temp = curr->next;
                                    curr->next = curr->next->next;
                                    free(temp);
                                }else{
                                    curr = curr->next;
                                }
                            }
                        }
                        ter = waitpid(0,NULL,WNOHANG);
                    }

                }
            }
        }else if(strcmp(argv[0], "bglist") == 0){
            /*print pid and command of the running background processes
             * by iterating through the linkedlist*/
            int count = 0;
            for (bg_pro* curr = root; curr != NULL; curr = curr->next){
                count++;
                printf("%d:   %s %d\n", curr->pid, curr->command, count);
            }
            printf("Total Background jobs:  %d\n", count);
        }else{
            /*execute arbitrary commands i.e ls, ./file*/
            pid_t pid = fork();
            if(pid==0){                 /*child*/
                if (execvp(argv[0],argv) < 0){
                    printf("case 2: invalid command\n");
                    exit(EXIT_FAILURE);
                    }
            }else{                      /*parent*/
                if(root != NULL){
                    /*after each iteration check if a child has terminated,
                     * if a child has terminated then remove it from the 
                     * linked list and print out which child has terminated.*/
                    pid_t ter = waitpid(0, NULL, WNOHANG);
                    while(ter > 0){
                        if(root->pid == ter){
                            printf("%d:   %s has terminated\n", root->pid, root->command);
                            bg_pro* temp = root;
                            root = root->next;
                            free(temp);
                        }else{
                            bg_pro* curr = root;
                            while (curr->next != NULL){
                                if(curr->next->pid == ter){
                                    printf("%d:   %s has terminated\n", curr->next->pid, curr->next->command);
                                    bg_pro* temp = curr->next;
                                    curr->next = curr->next->next;
                                    free(temp);
                                }else{
                                    curr = curr->next;
                                }
                            }
                        }
                        ter = waitpid(0,NULL,WNOHANG);
                    }

                }
                wait(NULL);
            }
        }
        if(root != NULL){
        /*after each iteration check if a child has terminated,
         * if a child has terminated then remove it from the 
         * linked list and print out which child has terminated.*/
            pid_t ter = waitpid(0, NULL, WNOHANG);
            while(ter > 0){
                if(root->pid == ter){
                    printf("%d:   %s has terminated\n", root->pid, root->command);
                    bg_pro* temp = root;
                    root = root->next;
                    free(temp);
                }else{
                    bg_pro* curr = root;
                    while (curr->next != NULL){
                        if(curr->next->pid == ter){
                            printf("%d:   %s has terminated\n", curr->next->pid, curr->next->command);
                            bg_pro* temp = curr->next;
                            curr->next = curr->next->next;
                            free(temp);
                        }else{
                            curr = curr->next;
                        }
                    }
                }
                ter = waitpid(0,NULL,WNOHANG);
            }

        }
        if(!strcmp(line, "exit")){
            if(root != NULL){
                deallocate(&root);
            }
            exit(0);
        }
    }
} 
