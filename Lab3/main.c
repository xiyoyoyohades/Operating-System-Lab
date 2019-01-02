#define _GNU_SOURCE

#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<sys/shm.h>
#include<sys/stat.h>
#include<stdlib.h>
#include<signal.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include"main.h"

int readprocessid=0;
int writeprocessid=0;
int read_position=0;
int write_position=0;
int ShmId;
int file_length_shmid;
int SemId;

int create_Sem(int key, int size) {
    int id;
    id = semget(key, size, IPC_CREAT | 0666);           //创建size个信号量
    if (id < 0) {                                       //判断是否创建成功
        printf("create sem %d,%d error\n", key, size);  //创建失败，打印错误
    }
    return id;
}

void set_sem_value(int semid, int index, int n) {
    union semun semopts;
    semopts.val = n;                                    //设定SETVAL的值为1
    semctl(semid, index, SETVAL, semopts);              //初始化信号量，信号量编号为0
    return;
}

void P(int semid, int index) {
    struct sembuf sem;                                  //信号量操作数组
    sem.sem_num = index;                                //信号量编号
    sem.sem_op = -1;                                    //信号量操作，-1为P操作
    sem.sem_flg = 0;                                    //操作标记：0或IPC_NOWAIT等
    semop(semid, &sem, 1);                              //1:表示执行命令的个数
    return;
}

void V(int semid, int index) {
    struct sembuf sem;                                  //信号量操作数组
    sem.sem_num = index;                                //信号量编号
    sem.sem_op =  1;                                    //信号量操作，1为V操作
    sem.sem_flg = 0;                                    //操作标记
    semop(semid, &sem, 1);                              //1:表示执行命令的个数
    return;
}

void showbar(size_t wrote, unsigned long filelength){
   // printf("line0 finish\n");
    //printf("%lu\n",filelength);
    int fillnum=(100*wrote/filelength);
    char bar[100];
    char* spin="-\\|/";
    memset(bar,'=',fillnum);
    memset(bar+fillnum,0,100-fillnum);
    printf("[%-100s][%d%%][%c]\r",bar,fillnum,spin[fillnum%4]);
    fflush(stdout);
    
}

int procRead(FILE* infile){
    block* assigned_blocks=(block*)shmat(ShmId,NULL,0);
    //char* read_exit="read exit!\n";
    while(1){
        P(SemId, 0);
        size_t bytesread=fread((assigned_blocks+read_position)->block_data, sizeof(char), DATA_SIZE, infile);
        (assigned_blocks+read_position)->numread=bytesread;
        if(bytesread<DATA_SIZE){
            read_position=(read_position+1)%BLOCK_NUM;
            fclose(infile);
            V(SemId, 1);
            //printf("%s",read_exit);
            return 0;
        }
        read_position=(read_position+1)%BLOCK_NUM;
        V(SemId, 1);
        
    }
}

int procWrite(FILE* outfile,unsigned long length){
    size_t totalwrote=0;
    //unsigned long length=0;
    block* assigned_blocks=(block*)shmat(ShmId,NULL,0);
    //unsigned long* filelength=(unsigned long*)shmat(file_length_shmid,NULL,0);
    //length=*filelength;
    //printf("%lu\n",length);
    while(1){
        P(SemId, 1);
        size_t realsize=(assigned_blocks+write_position)->numread;
        (assigned_blocks+write_position)->numread=0;
        
        if(realsize<DATA_SIZE){
            fwrite((assigned_blocks+write_position)->block_data, sizeof(char), realsize, outfile);
            totalwrote+=realsize*sizeof(char);
            showbar(totalwrote,length); printf("\n");
            write_position=(write_position+1)%BLOCK_NUM;
            fclose(outfile);
            V(SemId, 0);
            printf("write process exit!\nFile copied. %lu bytes in total\n",length);
            return 0;
        }
        fwrite((assigned_blocks+write_position)->block_data, sizeof(char), realsize, outfile);
        totalwrote+=realsize*sizeof(char);
        //printf("%lu\n",totalwrote);
        showbar(totalwrote,length);
        memset((assigned_blocks+write_position)->block_data, 0, DATA_SIZE);
        write_position=(write_position+1)%BLOCK_NUM;
        V(SemId, 0);
        
    }
}

unsigned long get_file_size(char* filename){
    unsigned long filesize=-1;
    struct stat statbuf;
    stat(filename,&statbuf);
    filesize=statbuf.st_size;
    //printf("%lu\n",filesize);
    return filesize;
}




    


int main(int argc, char *argv[]){
    if (argc != 3) {
        printf("Usage: %s <source-file> <dst-file>", argv[0]);
        return 1;
    }

    // check files' validity
    FILE *fileIn;
    FILE *fileOut;
    if ((fileIn = fopen(argv[1], "rb")) == NULL) {
        perror("failed to open file for reading.");
        return 1;
    }
    if ((fileOut = fopen(argv[2], "wb")) == NULL) {
        perror("failed to open file for writing,");
        return 1;
    }
    
    
    
    key_t shmkey;
    if ((shmkey = ftok("./", 'a')) == (key_t)(-1)) {
        perror("IPC error: ftok");
        exit(1);
    }
    
    
    if ((ShmId = shmget(shmkey, sizeof(block)*BLOCK_NUM, IPC_CREAT | 0660)) <= 0) {
        perror("Failed to create shared memory");
        exit(1);
    }
    
    block* assigned_blocks=(block*)shmat(ShmId,NULL,0);
    if ((int64_t)(assigned_blocks) == -1) {
        perror("Failed to retrieve shared memory");
        exit(1);
    }
    
    
    key_t semKey;
    if ((semKey = ftok("./", 'b')) == (key_t)(-1)) {
        perror("IPC error: ftok");
        exit(1);
    }
    // create lock for PV operation
    
    if ((SemId = create_Sem(semKey, 2)) < 0) {
        perror("Failed to create semaphore");
        exit(1);
    }
    
    unsigned long filelength=get_file_size(argv[1]);
    //printf("%lu\n",filelength);
    
    set_sem_value(SemId, 0, BLOCK_NUM);
    set_sem_value(SemId, 1, 0);
    
    if ((readprocessid = fork()) == -1) {
        perror("Failed to create read process.");
        return 1;
    }else if(readprocessid==0){
        return procRead(fileIn);
    }else{
        
        if ((writeprocessid= fork()) == -1) {
            perror("Failed to create write process.");
            // clean former threads
            kill(readprocessid, SIGKILL);
            return 1;
        }else if(writeprocessid==0){
            return procWrite(fileOut,filelength);
        }else{
            
            waitpid(readprocessid,NULL,0);
            waitpid(writeprocessid,NULL,0);
            
            semctl(SemId, 0, IPC_RMID, 0); 
            return 0;
        }
    }
}
    
    

    
