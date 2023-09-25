#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(){
	char *args[]={"sleep","7",NULL};
	pid_t p=fork();
	if(p==0){
		execvp(args[0],args);
	}
	else{
		while(1){
			printf("waitpid: %d\n",waitpid(p,NULL,1));
			sleep(1);
		}		
	}
}
