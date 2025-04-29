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

typedef struct process {
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
    int memsize, memoryused;
    struct Nodemem *mem;
} process;

process processList[MAX_PROCESSES];

struct msgbuff {
    long mtype;
    int msg;
};

//==============================
// Clock Communication Functions
//==============================
int getClk() {
    return *shmaddr;
}

void initClk() {
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1) {
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *)shmat(shmid, (void *)0, 0);
}

void destroyClk(bool terminateAll) {
    shmdt(shmaddr);
    if (terminateAll) {
        // Cleanup message queue as well
        msgctl(SendQueueID, IPC_RMID, NULL);
        killpg(getpgrp(), SIGINT);
    }
}

//==============================
// Input Handling
//==============================
int getNoOfProcessesFromInput(FILE *F) {
    char line[256];
    int processCount = 0;

    fgets(line, sizeof(line), F);

    while (fgets(line, sizeof(line), F)) {
        if (line[0] == '\n' || line[0] == '#') continue;
        processCount++;
    }

    fseek(F, 0, SEEK_SET);
    return processCount;
}

void readProcessesFromFile(FILE *f, int processCount) {
    int i = 0;
    char line[100];

    fgets(line, sizeof(line), f);

    while (fgets(line, sizeof(line), f) && i < processCount) {
        int id, arrival, runtime, priority;

        if (sscanf(line, "%d\t%d\t%d\t%d", &id, &arrival, &runtime, &priority) == 4) {
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
            processList[i].memsize = 0;
            processList[i].memoryused = 0;
            processList[i].mem = NULL;
            printf("Process %d:\t", i);
            printf("%d\t%d\t%d\t%d\n",processList[i].id,processList[i].arrivaltime,processList[i].runningtime,processList[i].priority);
            i++;
        }
    }

    if (i != processCount) {
        fprintf(stderr, "Warning: expected %d processes, but read %d\n", processCount, i);
    }
}

//==============================
// Scheduling Algorithm Chooser
//==============================
//==============================
// Message Sending Function
//==============================
void sendInfo(void) {
    int currentProcess = 0;
    struct msgbuff buf;

    key_t key = ftok("keyfile", 123); // Ensure keyfile exists
    SendQueueID = msgget(key, 0666 | IPC_CREAT);
    if (SendQueueID == -1) {
        perror("Failed to create scheduler message queue");
        exit(1);
    }

    while (currentProcess < NumberOfP) {
        int currentTime = getClk();

        if (processList[currentProcess].arrivaltime <= currentTime) {
            buf.mtype = processList[currentProcess].id;
            buf.msg = processList[currentProcess].remainingtime;

            if (msgsnd(SendQueueID, &buf, sizeof(buf.msg), 0) == -1) {
                perror("msgsnd failed");
            } else {
                printf("Sent process %d (remaining: %d) at time %d\n",
                       processList[currentProcess].id,
                       processList[currentProcess].remainingtime,
                       currentTime);
            }

            currentProcess++;
        } else {
            usleep(1000); // avoid busy waiting
        }
    }
}
