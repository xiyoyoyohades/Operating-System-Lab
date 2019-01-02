#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/sem.h>
/*
union semun {
   short val;               //SETVAL用的值
   struct semid_ds* buf;    //IPC_STAT、IPC_SET用的semid_ds结构
   unsigned short* array;   //SETALL、GETALL用的数组值
   struct seminfo *__buf;   //为控制IPC_INFO提供的缓存
};
*/


int semId;
int sum = 0;
pthread_t thread1, thread2;
int endflag=0;



void P(int semid, int index) {
    struct sembuf sem;                                  //array for ops of sem
    sem.sem_num = index;                                //index of sem
    sem.sem_op = -1;                                    //op of sem，-1 -> P
    sem.sem_flg = 0;                                    //flag of sem ops：0 or IPC_NOWAIT..
    semop(semid, &sem, 1);                              //1 -> how many times the op is to be done
    return;
}

void V(int semid, int index) {
    struct sembuf sem;                                  
    sem.sem_num = index;                                
    sem.sem_op =  1;                                    
    sem.sem_flg = 0;                                    
    semop(semid, &sem, 1);                              
    return;
}


void* calculate(void* parameter)
{
    for (size_t i = 1; i <= 100; i++)
    {
        P(semId, 1);
        sum+=i;
        if(i==100) endflag=1;
        V(semId, 0);
    }

    //endflag=1;
    return NULL;

}

void* print(void* parameter)
{
    while (1) {
        P(semId, 0);
        printf("sum = %d\n", sum);

        // try to wait for thread 1
        if (endflag) {
            pthread_join(thread1, NULL);
            return NULL;
        }

        V(semId, 1);
    }
    return NULL;

}

int main(int argc, char const *argv[])
{
    key_t semkey;
    if ((semkey = ftok("./main_thread.c", 'a')) == (key_t)(-1)) {       //generate a key for IPC to avoid key conflict
        perror("IPC error: ftok");
        exit(1);
    }
     
    semId = semget(semkey, 2, IPC_CREAT | 0666);
    
    //sem with index=0 -> if up to date
    //sem with index=1 -> if out dated
    semctl(semId, 0, SETVAL, 0);                                        //set the first sem to 0
    semctl(semId, 1, SETVAL, 1);                                        //set the second sem to 1
    
    pthread_create(&thread1, NULL, calculate, NULL);                    // create calculate thread
    pthread_create(&thread2, NULL, print, NULL);                        // create print thread

    pthread_join(thread2, NULL);                                        //wait for print thread to end, as the printer wait for the calculator

    semctl(semId, 0, IPC_RMID, 0);                                      //remove the sems

    return 0;
}
