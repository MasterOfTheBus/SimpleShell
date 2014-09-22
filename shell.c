#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <wait.h>

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
    int historyIndex = 0;

    while (1) { /* Program terminates normally inside setup */
	background = 0;
	printf(" COMMAND->\n");
	setup(inputBuffer,args,&background); /* get next command */
	/* the steps are:
	   (1) fork a child process using fork()
	   (2) the child process will invoke execvp()
	   (3) if background == 1, the parent will wait, 
	   otherwise returns to the setup() function. */
    // filter for &
/*    int i = 0;
    char *filtered[MAX_LINE + 1];
    while (i < MAX_LINE) {
        if (args[i] && args[i] == "-lart") {
            printf("found");
        }
        filtered[i] = args[i];
        i++;
    }
*/
	pid_t pid = fork();	
	if (pid == 0) {
        if (args[0] == "r") {
            // history part
            if (args[1] != 0) {
                
            }
        } else {
	        // check for &
            int i = 1;
/*        if (background) {
            fprintf(stderr, "the & was used\n");
        }*/
            while (i < MAX_LINE && args[i] != 0) {
           /* if (args[i] == '\0') {
                fprintf(stderr, "the \\0 char\n");
            } else {
                fprintf(stderr, "args: %s\n", args[i]);
            }*/
                i++;
            }
            char *options[i-1];
            int j = 0;
            int numOpt = (background) ? (i-2) : (i-1);
            while (j < numOpt) {
                options[j] = args[j+1];
                //fprintf(stderr, "option: %s\n", options[j]);
                j++;
            }
            j = 0;
            while (j < i-1) {
                fprintf(stderr, "option: %s\n", options[j]);
                j++;
            }
	        execvp(args[0], args);//options);
        }
	} else if (background == 1) {
        //int status;
        //waitpid(pid, &status, 0);
	}
    }
}
