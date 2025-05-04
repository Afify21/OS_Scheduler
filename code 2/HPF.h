#ifndef HPF_H
#define HPF_H
#include "MinHeap.h"
#include <errno.h>
#include <string.h>

static ssize_t rec;

void runHPF(int ProcessesCount) {
    printf("HPF: Starting with %d processes\n", ProcessesCount);

    MinHeap *readyQueue = createMinHeap(MAX_PROCESSES);
    process *currentProcess = NULL;
    int receivedProcesses = 0;
    int completedProcesses = 0;
    int lastClockTime = -1;
    int remainingTime = 0;

    while (completedProcesses < ProcessesCount || !HeapisEmpty(readyQueue) || currentProcess) {
        struct msgbuff nmsg;
        ssize_t rec = msgrcv(SendQueueID, &nmsg, sizeof(nmsg.msg), 0, IPC_NOWAIT);

        if (rec != -1) {
            printf("Scheduler: Received process %d at time %d\n", nmsg.msg.id, nmsg.msg.arrivaltime);
            insertMinHeap_HPF(readyQueue, nmsg.msg);
            receivedProcesses++;
            logEvent(nmsg.msg.arrivaltime, nmsg.msg.id, "arrived", nmsg.msg.arrivaltime, 
                     nmsg.msg.runningtime, nmsg.msg.runningtime, 0, 0, 0);
        }

        int currentTime = getClk()-1;
        if (currentTime != lastClockTime) {
            printf("\nCurrent Time: %d\n", currentTime); // Print current time each second
            lastClockTime = currentTime;

            // Check if current process has finished
            if (currentProcess) {
                remainingTime--;
                if (remainingTime <= 0) {
                    float TA = currentTime - currentProcess->arrivaltime;
                    float WTA = TA / currentProcess->runningtime;
                    logEvent(currentTime, currentProcess->id, "finished", 
                            currentProcess->arrivaltime, currentProcess->runningtime, 
                            0, TA - currentProcess->runningtime, TA, WTA);
                    printf("HPF: Process %d completed at time %d (TA: %.0f, WTA: %.2f)\n", 
                          currentProcess->id, currentTime, TA, WTA);
                    free(currentProcess);
                    currentProcess = NULL;
                    completedProcesses++;
                }
            }

            // Schedule new process if CPU is free
            if (!currentProcess && !HeapisEmpty(readyQueue)) {
                process p = deleteMinHPF(readyQueue);
                currentProcess = malloc(sizeof(process));
                *currentProcess = p;
                remainingTime = currentProcess->runningtime;
                logEvent(currentTime, currentProcess->id, "started", 
                        currentProcess->arrivaltime, currentProcess->runningtime, 
                        remainingTime, currentTime - currentProcess->arrivaltime, 0, 0);
                printf("HPF: Started process %d at time %d (Runtime: %d)\n", 
                      currentProcess->id, currentTime, remainingTime);
            }
        }
        usleep(1000); // Prevent busy-waiting
    }

    destroyMinHeap(readyQueue);
    printf("HPF: All %d processes completed\n", completedProcesses);
}
#endif