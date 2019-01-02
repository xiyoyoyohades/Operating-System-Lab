#define _POSIX_SOURCE


#include<string.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<signal.h>
#include<stdlib.h>
#include<stdio.h>

#define MAX_SIZE 1024

int child1(int piped[2]);
int child2(int piped[2]);
void parent_handler(int sig);
void child1_handler(int sig);
void child2_handler(int sig);

int child1pid, child2pid;
int* pipeptr;

int main(int argc, char *argv[]) {

    signal(SIGINT, parent_handler);

    int piped[2];
    pipeptr=piped;

    pipe(piped);

    // try fork the first child
    child1pid = fork();
    if (child1pid == -1) return -1;     //fork failed
    if (child1pid == 0) {               //if child process
        signal(SIGINT, SIG_IGN);        // ignore the interrupt signal
        signal(SIGUSR1, child1_handler);// register custom signal handler
        return child1(piped);
    }else{                              //if parent process
        child2pid = fork();
        if( child2pid == -1) return -1;     

        if (child2pid == 0) {               
                                        
            signal(SIGINT, SIG_IGN);        // ignore the interrupt signal                          
            signal(SIGUSR1, child2_handler);// register custom signal handler
            return child2(piped);
        }
    }

    while (1) {
        sleep(1);                           //parent sleep
    }
    return 0;
}

int child1(int piped[2]) {
    close(piped[0]);

    // the message buf
    char buf[MAX_SIZE];

    // run the main loop
    int counter= 1;
    // the pipe discriptor
    while (1) {
        sleep(1);                           //sleep 1 seconds
        sprintf(buf, "I send you %d times", counter);
        write(piped[1], buf, strlen(buf));
        counter++;
    }
}

int child2(int piped[2]) {
    // initialize the buf
    char buf[MAX_SIZE];
    // close unused pipe
    close(piped[1]);

    ssize_t size;
    while (1) {
        memset(buf, 0, MAX_SIZE);
        size = read(piped[0], buf, MAX_SIZE);
        // handle error
        if (size < 0)   return -1;
        buf[size] = 0;
        printf("%s\n", buf);
    }
}

void parent_handler(int sig) {
    kill(child1pid, SIGUSR1);
    kill(child2pid, SIGUSR1);
    int status;
    waitpid(child1pid, &status, 0);
    waitpid(child2pid, &status, 0);
    close(pipeptr[0]);
    close(pipeptr[1]);
    printf("Parent Process is Killed!\n");
    exit(0);
}

void child1_handler(int sig) {
    printf("Child Process 1 is Killed by Parent!\n");
    exit(0);
}

void child2_handler(int sig) {
    printf("Child Process 2 is Killed by Parent!\n");
    exit(0);
}
