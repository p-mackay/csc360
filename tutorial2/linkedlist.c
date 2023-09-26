#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc, char* argv[]){
    bg_pro* root = NULL;
    //root->pid = 15;
    //root->next = NULL;


    insert_end(&root, -2, "hello");
    insert_end(&root, 11, "world");
    insert_end(&root, 22, "hey");

    for (bg_pro* curr = root; curr != NULL; curr = curr->next){
        printf("%d\n", curr->pid);
        printf("%s\n", curr->command);
    }
    deallocate(&root);
    return 0;
}
