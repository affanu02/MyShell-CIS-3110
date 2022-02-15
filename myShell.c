/*
* Assignment 1, CIS*3110, due feb 5th, 2021. Create a shell.
* last date modified: 2021-02-10
* Author: Affan Khan 1095729
*/
#include <stdio.h>       /* Input/Output */
#include <stdlib.h>      /* General Utilities */
#include <unistd.h>      /* Symbolic Constants */
#include <sys/types.h>   /* Primitive System Data Types */
#include <sys/wait.h>    /* Wait for Process Termination */
#include <errno.h>       /* Errors */
#include <string.h>      /* Strings */
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <setjmp.h>

/*used for debugging purposes. Prints out the string array to output*/
void print_command_array(char **ptr){
    int i = 0;

    while(ptr[i] != NULL){
        printf("Array[%d]: %s\n", i, ptr[i++]);
    }
}

/*
* Takes a string and status, and parses the string from spaces and \n, putting the info into a 
* string array. status is increased to amount of elementes in the string array
*/
char** create_command_array(char *ptr, int *ptrstatus){
    /* remove the trailing \n */
    strtok(ptr,"\n");

    /*variables*/
    const char s[2] = " ";
    char **returnArray = NULL;
    char *token = strtok(ptr, s);
    int status = 1;
    int numberSpaces = 0;

    /* split string and append tokens to 'res' */
    while (token) {
        returnArray = realloc (returnArray, sizeof (char*) * ++numberSpaces);

        if (returnArray == NULL)
            exit (-1); /* memory allocation failed */

        returnArray[numberSpaces-1] = token;
        token = strtok (NULL, " ");
        if ( token != NULL ) {
            status++;
        }
    }

    /* realloc one extra element for the last NULL */
    returnArray = realloc (returnArray, sizeof (char*) * (numberSpaces+1));
    returnArray[numberSpaces] = 0;
    *ptrstatus = status;

    return returnArray;
}

/*
* Takes the string array, and status to redirect command line out into a file
* If file does not exist, it creates a new one. This is for the ">" in command lines. Must be followed by a file name.
*/
void output_redirection_file(char **ptr, int status){
    /*gets filename and removes > and filename from array*/
    char * filename = ptr[status - 1];        
    ptr[status - 1] = NULL;
    ptr[status - 2] = NULL;

    /*creates and opens the file*/
    FILE *fp;
    fp = freopen(filename, "w", stdout);
    

    /*alternate way of executing this function*/
    /*mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    //int outFile = open(filename, O_WRONLY|O_CREAT|O_TRUNC, mode);
    //dup2(outFile, 1);*/

    /*Executes and closes*/
    execvp(ptr[0], ptr);
    fclose(fp);
}

/*
* takes the string array, and status, to redirect the input for a command from a specified file.
* this is for the "<" part of the command line
*/
void input_redirection_file(char **ptr, int status){
    /*gets file name, removes < and file name from array*/
    char * filename = ptr[status -1];
    ptr[status - 1] = NULL;
    ptr[status - 2] = NULL;

    /*read only filename created*/
    int inFile = open(filename, O_RDONLY, 0);
    if(inFile == -1){
        perror(filename);
        exit(-1);
    }

    dup2(inFile, STDIN_FILENO);
    execvp(ptr[0], ptr);
}

/*
* takes in string array, and status. splits the array where "|" resides.
* executes the first half before "|" and takes it output and redirects its as the input
* for the second half after "|".
*/
void piping_redirection(char **ptr, int status){
    /*variables used*/
    int i = 0;
    int j = 0;
    int checker = 0;
    int elementP = 0;
    char *duplicate[status];

    /*while loop to find the | function so we can split the two sides of command line*/
    while(ptr[i] != NULL){
        if(strncmp(ptr[i++], "|", 4) == 0){
            elementP = i - 1;
            checker ++;
        }
    }

    /*this splits it and creates two sections of the command line, ptr, is the one before |, duplicate is the one after*/
    i = 0;
    j = elementP;
    while(ptr[j] != NULL){
        duplicate[i++] = ptr[++j];
    }
    ptr[elementP] = NULL;

    /*if piping fails*/
    int des_p[2];
    if(pipe(des_p) == -1) {
        perror("Pipe failed");
        exit(1);
    }

    /*if fork succeed, change the output of ptr, into the input of duplicate*/
    if(fork() == 0){
        close(STDOUT_FILENO);  

        /*code from sources in textbook/online*/
        dup(des_p[1]);
        close(des_p[0]);  
        close(des_p[1]);

        execvp(ptr[0], ptr);
        perror("execvp of ls failed");
        exit(1);
    }
    if(fork() == 0){
        /*code from sources in textbook/online*/
        close(STDIN_FILENO);
        dup(des_p[0]);        
        close(des_p[1]);       
        close(des_p[0]);
        
        execvp(duplicate[0], duplicate);
        perror("execvp of wc failed");
        exit(1);
    }

    /*closes the piping helpers*/
    close(des_p[0]);
    close(des_p[1]);
    wait(0);
    wait(0);
}

/*
* main program. Forks, gets user input, branches to different helper functions
*/
int main ( int argc, char *argv[] ) {
    pid_t childpid;   /* child's process id */
    int status = 0;   /* for parent process: child's exit status */
    char *line;
    char ** commandLineA;
    size_t length = 0;
    ssize_t nread = 0;
    int background;    

    /*get input from user*/
    line = (char *)malloc(256);
    printf("> ");
    nread = getline(&line, &length, stdin);

    /*loop until exit is entered*/
    while ((strncmp(line,"exit",4) != 0)) {
        /*exit if the fork fails*/
        if((childpid = fork()) < 0){
            perror("fork");
            exit(-1);
        }

        /*fork succeeded*/
        if ( childpid == 0 ) {
            /*check for piping*/
            char * checker = strchr(line, '|');

            /*parses the command line into an array of strings*/
            commandLineA = create_command_array(line, &status);

            /*execute commands*/
            if(status > 2 && strncmp(commandLineA[status-2], ">", 4) == 0){
                output_redirection_file(commandLineA, status);
            }
            else if(status > 2 && strncmp(commandLineA[status-2], "<", 4) == 0){
                input_redirection_file(commandLineA, status);
            }
            else if(status > 1 && strncmp(commandLineA[status-1], "&", 4) == 0){
                commandLineA[status-1] = NULL;
                system(line);
                /*execvp(commandLineA[0], commandLineA);
                /*8888888888888888888*/
            }
            else if(status > 1 && (checker != NULL)){
                piping_redirection(commandLineA, status);
            }
            else if(execvp(commandLineA[0], commandLineA) < 0){
                if(strcmp(line, "\n") != 0){
                    perror(commandLineA[0]);
                    exit(1);
                }
            }
            exit(status);
        }
        else {/*parent process*/
            waitpid(childpid,&status,0);
        }
        
        /*continues reading in inout and looping*/
        printf("> ");
        nread = getline(&line, &length, stdin);

    }

    /*end output and program exit*/
    printf("myShell terminating\n\n[Process completed]\n");
    exit(0);
}