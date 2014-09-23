#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <wait.h>
#include <string.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */

/**
 * setup() reads in the next command line, separating it into distinct tokens
 * using whitespace as delimiters. setup() sets the args parameter as a 
 * null-terminated string.
 */
void setup(char inputBuffer[], char *args[],int *background)
{
    int length, /* # of characters in the command line */
	i, /* loop index for accessing inputBuffer array */
	start, /* index where beginning of next command parameter is */
	ct; /* index of where to place the next parameter into args[] */

    ct = 0;

    /* read what the user enters on the command line */
    length = read(STDIN_FILENO, inputBuffer, MAX_LINE); 
    start = -1;
    if (length == 0)
	exit(0); /* ^d was entered, end of user command stream */
    if (length < 0){
	perror("error reading the command");
	exit(-1); /* terminate with error code of -1 */
    }
    /* examine every character in the inputBuffer */
    for (i=0;i<length;i++) { 
	switch (inputBuffer[i]){
	    case ' ':
	    case '\t' : /* argument separators */
		    if(start != -1){
		        args[ct] = &inputBuffer[start]; /* set up pointer */
		        ct++;
		    }
		    inputBuffer[i] = '\0'; /* add a null char; make a C string */
		    start = -1;
		    break;
	    case '\n': /* should be the final char examined */
		    if (start != -1){
		        args[ct] = &inputBuffer[start]; 
		        ct++;
		    }
		    inputBuffer[i] = '\0';
		    args[ct] = NULL; /* no more arguments to this command */
		    break;
	    default : /* some other character */
		    if (start == -1)
		        start = i;
		    if (inputBuffer[i] == '&'){
		        *background = 1;
		        inputBuffer[i] = '\0';
		    }
	    }
    } 
    args[ct] = NULL; /* just in case the input line was > 80 */
} 

int main(void)
{
    char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
    int background; /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE+1]; /* command line (of 80) has max of 40 arguments */
    int MAX_HISTORY = 10;
    char *history[MAX_HISTORY];
    int historyCount = 0;

    while (1) { /* Program terminates normally inside setup */
	background = 0;
	printf(" COMMAND->\n");
	setup(inputBuffer,args,&background); /* get next command */
	/* the steps are:
	   (1) fork a child process using fork()
	   (2) the child process will invoke execvp()
	   (3) if background == 1, the parent will wait, 
	   otherwise returns to the setup() function. */
	pid_t pid = fork();	
	if (pid == 0) {
        if (args[0] == "r") {
            // history part
            if (args[1] != 0) {
                
            }
        } else {
	        // Only get the actual arguments
            int i = 0;
            while (i < MAX_LINE && args[i] != 0) {
                i++;
            }

            // save the command to history
            char command[MAX_LINE + 1];
            strcpy(command, "");

            int numOpt = (background) ? (i) : (i+1);
            char *options[numOpt];
            int j = 0;
            while (j < i) {
                options[j] = args[j];
                strcat(command, options[j]);
                strcat(command, " ");
                j++;
            }
            options[numOpt-1] = 0;

            if (background) {
                strcat(command, "&");
            }
            historyCount++;
            history[historyCount % MAX_HISTORY] = command;

            // by convention, first element is the command
            // the array must be terminated by a null pointer
	        execvp(args[0], options);
        }
	} else if (background == 1) {
        //int status;
        //waitpid(pid, &status, 0);
	}
    }
}
