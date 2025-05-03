#ifndef HEADERS_H
#define HEADERS_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>

#define MAX_PROCESSES 100
#define SHKEY 300

// Global Variables
int *shmaddr;
int NumberOfP;
int SendQueueID;
int ReceiveQueueID;
int ReadyQueueID;

typedef struct process
{
    int id;
    pid_t pid;
    int arrivaltime;
    int runningtime;
    int priority;
    int starttime;
    int endtime;
    int remainingtime;
    int waittime;
    int responsetime;
    int finishtime;
    int turnaroundtime;
    int lasttime;
    int flag;

} process;

process processList[MAX_PROCESSES]; // Declare processList as extern
struct msgbuff
{
    long mtype;
    process msg;
};
//==============================
// Clock Communication Functions
//==============================
int getClk()
{
    return *shmaddr;
}

void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *)shmat(shmid, (void *)0, 0);
}

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        // Cleanup message queue as well
        msgctl(SendQueueID, IPC_RMID, NULL);
        killpg(getpgrp(), SIGINT);
    }
}

//==============================
// Input Handling
//==============================
int getNoOfProcessesFromInput(FILE *F)
{
    char line[256];
    int processCount = 0;

    fgets(line, sizeof(line), F);

    while (fgets(line, sizeof(line), F))
    {
        if (line[0] == '\n' || line[0] == '#')
            continue;
        processCount++;
    }

    fseek(F, 0, SEEK_SET);
    return processCount;
}

void readProcessesFromFile(FILE *f, int processCount)
{
    int i = 0;
    char line[100];

    fgets(line, sizeof(line), f);

    while (fgets(line, sizeof(line), f) && i < processCount)
    {
        int id, arrival, runtime, priority;
        if (sscanf(line, "%d\t%d\t%d\t%d", &id, &arrival, &runtime, &priority) == 4)
        {
            processList[i].id = id;
            processList[i].arrivaltime = arrival;
            processList[i].runningtime = runtime;
            processList[i].priority = priority;
            processList[i].remainingtime = runtime;
            processList[i].starttime = -1;
            processList[i].endtime = -1;
            processList[i].pid = -1;
            processList[i].waittime = 0;
            processList[i].responsetime = -1;
            processList[i].turnaroundtime = -1;
            processList[i].finishtime = -1;
            processList[i].lasttime = -1;
            processList[i].flag = 0;
            printf("Process %d:\t", i);
            printf("%d\t%d\t%d\t%d\n", processList[i].id, processList[i].arrivaltime, processList[i].runningtime, processList[i].priority);
            i++;
        }
    }

    if (i != processCount)
    {
        fprintf(stderr, "Warning: expected %d processes, but read %d\n", processCount, i);
    }
}
void logEvent(int time, int pid, const char *state, int arrival, int total, int remain, int wait)
{
    FILE *log = fopen("scheduler.log", "a");
    fprintf(log, "At time %d process %d %s\n", time, pid, state);
    fclose(log);
}
// void DefineKeysProcess(int *SendQueueID, int *ReceiveQueueID)
// {
//     key_t sendKey = ftok("keyfile", 65);
//     key_t receiveKey = ftok("keyfile", 66);

//     *SendQueueID = msgget(sendKey, 0666 | IPC_CREAT);
//     *ReceiveQueueID = msgget(receiveKey, 0666 | IPC_CREAT);

//     if (*SendQueueID == -1 || *ReceiveQueueID == -1)
//     {
//         perror("Error creating message queues");
//         exit(-1);
//     }
// }
void DefineKeys(int *ReadyQueueID, int *SendQueueID, int *ReceiveQueueID)
{
    key_t ReadyQueueKey;
    ReadyQueueKey = ftok("keys/Funnyman", 'A');
    *ReadyQueueID = msgget(ReadyQueueKey, 0666 | IPC_CREAT);
    if (*ReadyQueueID == -1)
    {
        perror("Error in create message queue");
        exit(-1);
    }
    // Initialize Send queue to send turn to process
    key_t SendQueueKey;
    SendQueueKey = ftok("keys/Sendman", 'A');
    *SendQueueID = msgget(SendQueueKey, 0666 | IPC_CREAT);
    if (*SendQueueID == -1)
    {
        perror("Error in create message queue");
        exit(-1);
    }
    // Initialize Receive queue to receive remaining time from process
    key_t ReceiveQueueKey;
    ReceiveQueueKey = ftok("keys/Receiveman", 'A');
    *ReceiveQueueID = msgget(ReceiveQueueKey, 0666 | IPC_CREAT);
    if (*ReceiveQueueID == -1)
    {
        perror("Error in create message queue");
        exit(-1);
    }
    // Initialize GUI queue to send process information to GUI
}

//==============================
// Scheduling Algorithm Chooser
//==============================
//==============================
// Message Sending Function
//==============================

// syncing
int *Synchro;

int getSync()
{
    return *Synchro;
}
void setSync(int val)
{
    *Synchro = val;
}

void initSync()
{
    key_t key = ftok("keys/Syncman", 65);
    int Syncid = shmget(key, 4, IPC_CREAT | 0644);
    Synchro = (int *)shmat(Syncid, (void *)0, 0);
}

void destroySync(bool delete)
{
    shmdt(Synchro);
    if (delete)
    {
        key_t key = ftok("keys/Syncman", 65);
        int Syncid = shmget(key, 4, 0444);
        shmctl(Syncid, IPC_RMID, NULL);
    }
}
#endif
