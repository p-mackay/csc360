#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <readline/readline.h>
#include <readline/history.h>

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
	const char* prompt = cwd;


	int bailout = 0;
	while (!bailout) {
        
		char* reply = readline(prompt);
        if (strcmp(reply, "cd") == 0){
            chdir("/home/paul/");
            prompt = "/home/paul/";
        }
		//char* reply = readline(prompt);
		/* Note that readline strips away the final \n */
		/* For Perl junkies, readline automatically chomps the line read */

		if (!strcmp(reply, "bye")) {
			bailout = 1;
		} else {
			printf("\nYou said: %s\n\n", reply);
		}
	
		free(reply);
	}
	printf("Bye Bye\n");
}
