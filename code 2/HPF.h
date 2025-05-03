#ifndef HPF_H
#define HPF_H
#include "MinHeap.h"
#include <errno.h>

static ssize_t rec;
void runHPF(int ProcessesCount) {
    printf("HPF: Starting with %d processes\n", ProcessesCount);

    MinHeap *readyQueue = createMinHeap(MAX_PROCESSES);
    process *currentProcess = NULL;
    int receivedProcesses = 0;
    int lastClockTime = -1;
    int remainingTime = 0; // Track remaining time of current process

    while (receivedProcesses < ProcessesCount || !HeapisEmpty(readyQueue) || currentProcess) {
        struct msgbuff nmsg;
        ssize_t rec = msgrcv(SendQueueID, &nmsg, sizeof(nmsg.msg), 0, IPC_NOWAIT);

        if (rec != -1) {
            printf("Scheduler: Received process %d at time %d\n", nmsg.msg.id, getClk());
            insertMinHeap_HPF(readyQueue, nmsg.msg);
            receivedProcesses++;
        }

        int currentTime = getClk();
        if (currentTime != lastClockTime) {
            lastClockTime = currentTime;

            // Check if current process has finished
            if (currentProcess) {
                remainingTime--;
                if (remainingTime <= 0) {
                    printf("HPF: Process %d completed at time %d\n", currentProcess->id, currentTime);
                    currentProcess = NULL;
                }
            }

            // Schedule new process if CPU is free
            if (!currentProcess && !HeapisEmpty(readyQueue)) {
                process p = deleteMinHPF(readyQueue);
                currentProcess = malloc(sizeof(process));
                *currentProcess = p;
                remainingTime = currentProcess->runningtime;
                printf("HPF: Started process %d at time %d (Runtime: %d)\n", 
                      currentProcess->id, currentTime, remainingTime);
            }
        }
        usleep(1000); // Reduce CPU usage
    }

    destroyMinHeap(readyQueue);
    printf("HPF: All processes completed\n");
}
#endif