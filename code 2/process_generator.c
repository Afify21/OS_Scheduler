// process_generator.c
#include "headers.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/msg.h>
#include <string.h>

// Globals
int algoChoice;
int quantum = -1;
int ReadyQueueID, SendQueueID, ReceiveQueueID;
int NumberOfP;
process processList[MAX_PROCESSES];

void clearResources(int signum) {
    msgctl(ReadyQueueID,   IPC_RMID, NULL);
    msgctl(SendQueueID,    IPC_RMID, NULL);
    msgctl(ReceiveQueueID, IPC_RMID, NULL);
    destroyClk(true);
    exit(0);
}

void chooseAlgorithm(void) {
    printf("Choose a scheduling algorithm:\n"
           "1. HPF\n"
           "2. SRTN\n"
           "3. RR\n"
           "Enter (1-3): ");
    if (scanf("%d", &algoChoice) != 1 || algoChoice < 1 || algoChoice > 3) {
        fprintf(stderr, "Invalid choice\n");
        exit(1);
    }
    if (algoChoice == 3) {
        printf("Enter time quantum: ");
        if (scanf("%d", &quantum) != 1 || quantum <= 0) {
            fprintf(stderr, "Invalid quantum\n");
            exit(1);
        }
    }
}

void setUP_CLK_SCHDLR(void) {
    // Clean up any old queues
    key_t k1 = ftok("keyfile", 65), k2 = ftok("keyfile", 66);
    int t1 = msgget(k1, 0666), t2 = msgget(k2, 0666);
    if (t1 != -1) msgctl(t1, IPC_RMID, NULL);
    if (t2 != -1) msgctl(t2, IPC_RMID, NULL);

    // Create the three queues
    DefineKeys(&ReadyQueueID, &SendQueueID, &ReceiveQueueID);

    // Fork clock
    if (fork() == 0) {
        execl("./clk.out", "clk.out", NULL);
        perror("clk.execl");
        _exit(1);
    }
    sleep(1);

    // Init clock in parent
    initClk();

    // Fork scheduler
    if (fork() == 0) {
        char p[16], a[4], q[4];
        sprintf(p, "%d", NumberOfP);
        sprintf(a, "%d", algoChoice);
        sprintf(q, "%d", quantum);
        execl("./scheduler.out", "scheduler.out", p, a, q, NULL);
        perror("sched.execl");
        _exit(1);
    }
    sleep(1);
}

void sendInfo2(void) { ///////////////////////////RR
    struct msgbuff buf;
    int sent = 0;
    while (sent < NumberOfP) {
        int clk = getClk();
        if (processList[sent].arrivaltime <= clk) {
            buf.mtype = 1;  // a single arrival queue type
            memcpy(&buf.msg, &processList[sent], sizeof(processList[sent]));
            if (msgsnd(ReadyQueueID, &buf, sizeof(buf.msg), 0) == -1) {
                perror("msgsnd ReadyQueue");
            }
            sent++;
        } else {
            usleep(100000);
        }
    }
    // Keep generator alive so IPC remains valid
    for (;;)
        pause();
}
void sendInfo(void) {           ///////////////////////////HPF and SRTN
    int currentProcess = 0;
    struct msgbuff buf;

    // We've already created the message queues in setUP_CLK_SCHDLR

    printf("Starting to send process information to scheduler...\n");
    while (currentProcess < NumberOfP) {
        int currentTime = getClk();

        if (processList[currentProcess].arrivaltime <= currentTime)
        {
            buf.mtype = processList[currentProcess].id;
            buf.msg = processList[currentProcess]; // Copy entire struct
            buf.msg.lasttime = currentTime;


            if (msgsnd(SendQueueID, &buf, sizeof(buf.msg), 0) == -1)
            {
                perror("msgsnd failed");
            } else {
                printf("Sent process %d (remaining: %d, priority: %d, running: %d) at time %d\n",
                    processList[currentProcess].id,
                    processList[currentProcess].remainingtime,
                    processList[currentProcess].priority,
                    processList[currentProcess].runningtime,
                    currentTime);
            }

            currentProcess++;
        }
        else
        {
            // Sleep for a short time to avoid busy waiting
            usleep(100000); // 100ms sleep is more reasonable
        }
    }

    printf("All processes have been sent to the scheduler\n");
    while(1);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }
    signal(SIGINT, clearResources);

    FILE *f = fopen(argv[1], "r");
    if (!f) { perror("fopen"); return 1; }
    NumberOfP = getNoOfProcessesFromInput(f);
    readProcessesFromFile(f, NumberOfP);
    fclose(f);

    chooseAlgorithm();
    setUP_CLK_SCHDLR();
    if(quantum != -1)
        sendInfo2();
    else
    sendInfo();

    destroyClk(true);
    return 0;
}
