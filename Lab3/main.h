#include<stdio.h>
#include<stdint.h>


#define DATA_SIZE 1024
#define BLOCK_NUM 1024

typedef struct block{
    size_t numread;
    char block_data[DATA_SIZE];
}block;

union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO */
};




