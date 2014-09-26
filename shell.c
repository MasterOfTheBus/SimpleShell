#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <wait.h>
#include <string.h>
#include <linux/limits.h>
#include <ctype.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define MAX_HISTORY 10 /* Max of 10 commands in the history buffer */
#define MAX_JOBS 50 /* Max number of jobs that can be run in the background */
#define CD "cd"
#define PWD "pwd"
#define EXIT "exit"
#define JOBS "jobs"
#define FG "fg"
#define HISTORY "history"
#define R "r"

typedef struct {
    pid_t pid;
    char *command;
} job;

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
        //printf("input: %s\n", inputBuffer);
        length = strlen(inputBuffer);
        //printf("length: %d\n", length);
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
    return (ct+1);
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

void runCmd(char *args[], int argsCount, int background, char command[],
            job jobs[], int jobCount)
{
    int status;
    pid_t pid = fork();
    if (pid == 0) {
        int numOpt = (background) ? (argsCount-1) : (argsCount);
        char *options[numOpt];
        int j = 0;
        while (j < argsCount) {
            options[j] = args[j];
            j++;
        }
        options[numOpt-1] = 0;

        // by convention, first element is the command
        // the array must be terminated by a null pointer
        execvp(args[0], options);
    } else {
        if (background) {
            int ret = waitpid(pid, &status, WNOHANG);
            if (ret > 0) {
                printf("Done");
            } else if (ret == 0) {
                job newJob;
                newJob.command = strdup(command);
                newJob.pid = pid;
                jobs[jobCount - 1] = newJob;
                printf("[%d] %d\n", jobCount, newJob.pid);
            } else {
                perror("Some error");
                return;
            }
        } else {
            wait(&status);
        }
    }
}

/*
 * Return 1 for true and 0 for false
 */
int isSystemCall(char *command)
{
    if (strcmp(command, CD) == 0 || strcmp(command, PWD) == 0 ||
        strcmp(command, EXIT) == 0 || strcmp(command, HISTORY) == 0) {
        return (1);
    }
    return (0);
}

void removeJob(job jobs[], int jobId, int jobCount)
{
    int i;
    for (i = jobId; i < jobCount; i++) {
        if (i == jobCount - 1) {
            jobs[i].command = "\0";
            jobs[i].pid = -1;
        } else {
            jobs[i] = jobs[i+1];
        }
    }
}

int displayJobs(job jobs[], int jobCount, int doneOnly) {
    int i, status, ret, numDone = 0;
    for (i = 0; i < jobCount; i++) {
        ret = waitpid(jobs[i].pid, &status, WNOHANG);
        if (ret == 0) {
            if (!doneOnly) {
                printf("[%d] %d Running    %s\n", i+1, jobs[i].pid, jobs[i].command);
            }
        } else {
            printf("[%d] %d Done    %s\n", i+1, jobs[i].pid, jobs[i].command);
            numDone++;
            removeJob(jobs, i, jobCount);
            i--;
        }
    }
    return (numDone);
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
    } else if (strcmp(args[0], HISTORY) == 0) {
        int i;
        int limit = (historyCount > MAX_HISTORY) ? (MAX_HISTORY) : (historyCount);
        for (i = 0; i < limit; i++) {
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
    job jobs[MAX_JOBS];
    int jobCount = 0;
    int argsCount = 0;

    while (1) { /* Program terminates normally inside setup */
    	background = 0;
	    printf(" COMMAND->\n");
        /* get next command */
        argsCount = setup(inputBuffer,args,&background,1); 
	    if(argsCount == -1) {
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
                while (i < argsCount && args[i] != 0) {
                    strcat(command, args[i]);
                    strcat(command, " ");
                    i++;
                }
                if (background) {
                    command[strlen(command)-1] = '&';
                    jobCount++;
                    if (background && jobCount >= MAX_JOBS) {
                        printf("Number of background jobs exceeds the limit");
                        jobCount--;
                        continue;
                    }
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
                if (strlen(args[1]) != 1) {
                    printf("Invalid argument; argument must be one character\n");
                    continue;
                }
                int i;
                int limit = (historyCount > MAX_HISTORY) ? (MAX_HISTORY) :
                    (historyCount);
                char tempArg[5];
                strcpy(tempArg, args[1]);
                for (i = limit - 1; i >= 0; i--) {
                    char temp[MAX_LINE+1];
                    strcpy(temp, history[i]);
                    if (temp[0] == tempArg[0]) {
                        strcpy(command, history[i]);
                        printf("%s\n", command);
                        break;
                    }
                }
            } else {
                // The most recent command
                int index = (historyCount < MAX_HISTORY) ? (historyCount - 1) :
                    (MAX_HISTORY - 1); 
                strcpy(command, history[index]);
                printf("%s\n", command);
            }
            historyCount++;
            addCommand(history, command, historyCount);
            argsCount = setup(command, args, &background,0);
            if (argsCount == -1) {
                continue;
            }
            /*int z = 0;
            while(args[z] != 0) {
                printf("args%d: %s\n",z,args[z]);
                z++;
            }*/
        } else if (strcmp(args[0], FG) == 0) {
            int status, index;
            pid_t pid, ret;
            if (args[1] != 0) {
                if (isdigit(args[1])) {
                    index = atoi(args[1]);
                    pid = jobs[index].pid;
                } else {
                    printf("%s: no such job\n", args[1]);
                    continue;
                }
            } else {
                index = jobCount - 1;
                pid = jobs[index].pid; 
            }
            ret = waitpid(pid, &status, 0);
            if (ret == pid) {
                removeJob(jobs, index, jobCount);
                jobCount--;
            }
        }
        int doneOnly = 0;
        if (isSystemCall(args[0])) {
            doneOnly = 1;
            runSystemCall(args, historyCount, history);
        } else if (strcmp(args[0], JOBS)) {
            doneOnly = 1;
            runCmd(args, argsCount, background, command, jobs, jobCount);
        }
        // display finished jobs
        int finished = displayJobs(jobs, jobCount, doneOnly);
        jobCount -= finished;
    }
}
