#include <stdio.h> //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

typedef short bool;
#define true 1
#define false 0
#define MAX_PROCESSES 100  

#define SHKEY 300
int SendQueueID;
///==============================
// don't mess with this variable//
int *shmaddr; //
//===============================

struct process {
    int id;
    int arrivalTime;
    int remainingTime;
    int priority;
};

struct process processList[MAX_PROCESSES]; 

int getClk()
{
    return *shmaddr;
}

/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
 */
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        // Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *)shmat(shmid, (void *)0, 0);
}

/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
 */

int getNoOfProcessesFromInput(FILE* F)
{
    int charc;
    int Processes=0;

    while((charc=getc(F))!=EOF)
    {
        if ( charc=='\n')
        {
            Processes++;
        }
       
    }

    fseek(F,0,SEEK_SET);//return pointer to start
    return Processes;
}


void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}
void chooseAlgorithm(void)
{
    int algoChoice;
    int quantum = -1; // default value

    printf("Choose a scheduling algorithm:\n");
    printf("1. HPF (Highest Priority First)\n");
    printf("2. SRTN (Shortest Remaining Time)\n");
    printf("3. RR  (Round Robin)\n");
    printf("Enter your choice (1-3): ");
    scanf("%d", &algoChoice);

    if (algoChoice == 3) // RR needs quantum
    {
        printf("Enter the time quantum: ");
        scanf("%d", &quantum);
    }

    // Example to print what was selected
    if (algoChoice == 1)
        printf("You selected HPF.\n");
    else if (algoChoice == 2)
        printf("You selected SRT.\n");
    else if (algoChoice == 3)
        printf("You selected RR with quantum = %d.\n", quantum);
    else
    {
        printf("Invalid choice. Exiting.\n");
        exit(1);
    }
}
void sendInfo(int numberOfProcesses) {
    int currentProcess = 0;
    struct msgbuff buf;
    // Inside processgen.c's main() or init function:
    key_t key = ftok("your_unique_file", 123); // Use a unique identifier
     SendQueueID = msgget(key, 0666 | IPC_CREAT);

if (SendQueueID == -1) {
    perror("Failed to create scheduler message queue");
    exit(1);
}

    while (currentProcess < numberOfProcesses) {
        int currentTime = getClk();

        // Check if the current process has arrived
        if (processList[currentProcess].arrivalTime <= currentTime) {
            // Prepare the message
            buf.mtype = processList[currentProcess].id; // Use PID as message type
            buf.msg = processList[currentProcess].remainingTime;

            // Send the message to the scheduler's queue
            if (msgsnd(SendQueueID, &buf, sizeof(buf.msg), 0) == -1) {
                perror("msgsnd failed");
            } else {
                printf("Sent process %d (remaining: %d) at time %d\n",
                       processList[currentProcess].id,
                       processList[currentProcess].remainingTime,
                       currentTime);
            }

            currentProcess++; // Move to the next process
        } else {
            usleep(1000); // Avoid busy waiting
        }
    }
}

struct msgbuff
{
    long mtype;
    int msg;
};
