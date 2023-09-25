#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
int main(){
	printf("PID:%d\n",getpid());
	int p=fork();
	printf("PID again:%d, p=%d\n",getpid(),p);
	return 0;
}
