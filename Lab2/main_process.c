#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h> 
#include <sys/types.h>
#include <sys/sem.h>

int semId;
int shmid;
int cal_proc_id=0;
int print_proc_id=0;

void P(int semid, int index) {
    struct sembuf sem;                                  //array for ops of sem
    sem.sem_num = index;                                //index of sem
    sem.sem_op = -1;                                    //op of sem，-1 -> P
    sem.sem_flg = 0;                                    //flag of sem ops：0 or IPC_NOWAIT..
    semop(semid, &sem, 1);                              //1 -> how many times the op is to be done
    return;
}

void V(int semid, int index) {
    struct sembuf sem;                                  //array for ops of sem
    sem.sem_num = index;                                //index of sem
    sem.sem_op =  1;                                    //op of sem，1 -> V
    sem.sem_flg = 0;                                    //flag of sem ops：0 or IPC_NOWAIT..
    semop(semid, &sem, 1);                              //1 -> how many times the op is to be done
    return;
}

int calculate_proc(){
    int* sharedsum=(int*)shmat(shmid, NULL, 0);         //connect shared memory to process space
    if((int64_t)(sharedsum)==-1){
        perror("Fail to retrieve shared memory\n");
        exit(1);
    }
    *(sharedsum)=0;                                     //initialize sum
    *(sharedsum+1)=0;                                   //initialize flag of end
    for (size_t i = 1; i <= 100; i++)
    {
        P(semId, 1);
        (*sharedsum)+=i;
        if(i==100){                                     //set the flag of end, then release printer
            *(sharedsum+1)=1;
        }
        V(semId, 0);
    }
    shmdt(sharedsum);                                   //cancel the usage of the shared memory
    printf("Calculator process exit\n");
    return 0;
}

int printer_proc(){
    int* sharedsum=(int*)shmat(shmid, NULL, 0);
    if((int64_t)(sharedsum)==-1){
        perror("Fail to retrieve shared memory\n");
        exit(1);
    }
    while (1) {
        P(semId, 0);
        printf("sum = %d\n", *sharedsum);
        if (*(sharedsum+1)){
            printf("Printer process exit\n");           //only if the end flag==1, end process
            return 0;
        }
        V(semId, 1);
    }
    shmdt(sharedsum);
    
    return 0;

}

int main(int argc, char const *argv[])
{
    key_t semkey;
    if ((semkey = ftok("./main_process.c", 'a')) == (key_t)(-1)) {      //generate a key for IPC to avoid key conflict
        perror("IPC error: ftok");
        exit(1);
    }
     
    semId = semget(semkey, 2, IPC_CREAT | 0666);
    
    //sem with index=0 -> if up to date
    //sem with index=1 -> if out dated
    semctl(semId, 0, SETVAL, 0);                        //set the first sem to 0
    semctl(semId, 1, SETVAL, 1);                        //set the second sem to 1

    key_t shmkey;
    if ((shmkey = ftok("./main_process.c", 'b')) == (key_t)(-1)) {
        perror("IPC error: ftok");
        exit(1);
    }
    // create the shared memory and initialize it
    // first int -> sum (initialized with 0)
    // second int -> flag for end (1 for end)
    
    if ((shmid = shmget(shmkey, sizeof(int)*2, IPC_CREAT | 0660)) <= 0) {
        perror("Failed to create shared memory");
        exit(1);
    }

    int* sharedsum=(int*)shmat(shmid, NULL, 0);
    if((int64_t)(sharedsum)==-1){
        perror("Fail to retrieve shared memory\n");
        exit(1);
    }

    if((cal_proc_id=fork())==-1){
        perror("Failed to create calculaor process\n");
        return 1;
    }
    else if(cal_proc_id==0) return calculate_proc();                    //if is in the child process
    else{
        if((print_proc_id=fork())==-1){
            perror("Failed to create printer process\n");
                                                                        //kill the first child process if the second can't be created
            kill(cal_proc_id, SIGKILL);
            return 1;
        }
        else if(print_proc_id==0) return printer_proc();                //if is in the child process
        else{                                                           //if is in the parent process
            waitpid(cal_proc_id, NULL, 0);                              //wait for child process to exit
            waitpid(print_proc_id, NULL, 0);
            if(semctl(semId, 0, IPC_RMID, 0)<0) perror("Destroy sem error\n");                  // destroy sem
            if(shmctl(shmid, IPC_RMID, 0) == -1) perror("Failed to remove shared memory\n");    // destroy shared memory

        }
    }

    
    printf("Parent process exit\n");
    return 0;
}

