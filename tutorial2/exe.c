#include <unistd.h>
int main() {
    while(1){
        char* args[3];
        args[0]="ls";
        args[1]="-l";
        args[2]=NULL;
        execvp(args[0],args);
    }
    return 0;
}
