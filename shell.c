#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <wait.h>
#include <string.h>
#include <linux/limits.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define MAX_HISTORY 10 /* Max of 10 commands in the history buffer */
#define CD "cd"
#define PWD "pwd"
#define EXIT "exit"
#define JOBS "jobs"
#define FG "fg"
#define HISTORY "history"
#define R "r"

/**
 * setup() reads in the next command line, separating it into distinct tokens
 * using whitespace as delimiters. setup() sets the args parameter as a 
 * null-terminated string.
 */
int setup(char inputBuffer[], char *args[],int *background, int readInput)
{
    int length, /* # of characters in the command line */
	i, /* loop index for accessing inputBuffer array */
	start, /* index where beginning of next command parameter is */
	ct; /* index of where to place the next parameter into args[] */

    ct = 0;
    if (readInput) {
        /* read what the user enters on the command line */
        length = read(STDIN_FILENO, inputBuffer, MAX_LINE);
    } else {
        printf("input: %s\n", inputBuffer);
        length = strlen(inputBuffer);
        printf("length: %d\n", length);
    }
    start = -1;
    if (length == 0)
	exit(0); /* ^d was entered, end of user command stream */
    if (length < 0){
	perror("error reading the command");
	return (-1); /* terminate with error code of -1 */
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
    return (0);
} 

void addCommand(char *history[], char command[], int historyCount) {
    if (historyCount <= MAX_HISTORY) {
        history[historyCount - 1] = strdup(command);
    } else {
        int i;
        for (i = 0; i < MAX_HISTORY - 1; i++) {
            history[i] = history[i + 1];
        }
        history[MAX_HISTORY - 1] = strdup(command);
    }
}

void runCmd(char *args[], int background)
{
    pid_t pid = fork();
    if (pid == 0) {
        // Only get the actual arguments
        int i = 0;
        while (i < MAX_LINE && args[i] != 0) {
            i++;
        }

        // save the command to history
        int numOpt = (background) ? (i) : (i+1);
        char *options[numOpt];
        int j = 0;
        while (j < i) {
            options[j] = args[j];
            j++;
        }
        options[numOpt-1] = 0;

        // by convention, first element is the command
        // the array must be terminated by a null pointer
        execvp(args[0], options);
    } else if (background == 1) {
        int status;
        waitpid(pid, &status, 0);
    }
}

/*
 * Return 1 for true and 0 for false
 */
int isSystemCall(char *command)
{
    if (strcmp(command, CD) == 0 || strcmp(command, PWD) == 0 ||
        strcmp(command, EXIT) == 0 || strcmp(command, FG) == 0 ||
        strcmp(command, JOBS) == 0 || strcmp(command, HISTORY) == 0) {
        return (1);
    }
    return (0);
}

void runSystemCall(char *args[], int historyCount, char *history[])
{
    if (strcmp(args[0], CD) == 0){
        chdir(args[1]);
    } else if (strcmp(args[0], PWD) == 0) {
        char buf[PATH_MAX];
        getcwd(buf, PATH_MAX);
        if (buf != 0) {
            printf("%s\n", buf);
        } else {
            printf("Error getting current working directory\n");
        }
    } else if (strcmp(args[0], EXIT) == 0) {
        exit(0);
    } else if (strcmp(args[0], JOBS) == 0) {

    } else if (strcmp(args[0], FG) == 0) {

    } else if (strcmp(args[0], HISTORY) == 0) {
        int i;
        for (i = 0; i < MAX_HISTORY; i++) {
            int count = (historyCount > MAX_HISTORY) ?
                (historyCount - MAX_HISTORY + i + 1) : (i + 1);
            printf(" %d\t%s\n", count, history[i]);
        }
    }
}

int main(void)
{
    char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
    int background; /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE+1]; /* command line (of 80) has max of 40 arguments */
    char *history[MAX_HISTORY];
    int historyCount = 0;
    char command[MAX_LINE+1];

    while (1) { /* Program terminates normally inside setup */
    	background = 0;
	    printf(" COMMAND->\n");
        /* get next command */
	    if(setup(inputBuffer,args,&background,1) != 0) {
            continue;
        }
	    /* the steps are:
	        (1) fork a child process using fork()
	        (2) the child process will invoke execvp()
	        (3) if background == 1, the parent will wait, 
	        otherwise returns to the setup() function. */
	    
        // add the command to the history
        // Only get the actual arguments
        if (args[0] != 0) {
            if (strcmp(args[0], R)) {
                strcpy(command, "");

                int i = 0;
                while (i < MAX_LINE && args[i] != 0) {
                    strcat(command, args[i]);
                    strcat(command, " ");
                    i++;
                }
                if (background) {
                    command[strlen(command)-1] = '&';
                }
                historyCount++;
                addCommand(history, command, historyCount);
            }
        } else {
            continue;
        }
        if (strcmp(args[0], R) == 0) {
            if (args[1] != 0) {
                // A specified command
            } else {
                // The most recent command
                int index = (historyCount < MAX_HISTORY) ? (historyCount - 1) :
                    (MAX_HISTORY - 1); 
                strcpy(command, history[index]);
                char *tempCmd = strdup(command);
                historyCount++;
                addCommand(history, command, historyCount);
                if (setup(command, args, &background,0) != 0) {
                    continue;
                }
                printf("%s\n", tempCmd);
                int z = 0;
                while(args[z] != 0) {
                    printf("args%d: %s\n",z,args[z]);
                    z++;
                }
                runCmd(args, background);
            }
        } else if (isSystemCall(args[0])) {
            runSystemCall(args, historyCount, history);
        } else {
            runCmd(args, background);
        }
    }
}
