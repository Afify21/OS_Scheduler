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
    float TAs[ProcessesCount];         // Turnaround times
    float WTAs[ProcessesCount];        // Weighted turnaround times
    int taIdx = 0;
    int sumRun = 0;
    int sumWait = 0;
    float sumWTA = 0.0f;

    while (completedProcesses < ProcessesCount || !HeapisEmpty(readyQueue) || currentProcess) {
        struct msgbuff nmsg;
        ssize_t rec = msgrcv(SendQueueID, &nmsg, sizeof(nmsg.msg), 0, IPC_NOWAIT);

        if (rec != -1) {
            printf("Scheduler: Received process %d at time %d\n", nmsg.msg.id, nmsg.msg.arrivaltime);
            // Fork and execute process.out
            pid_t pid = fork();
            if (pid == 0) {
                char runtimeArg[16];
                sprintf(runtimeArg, "%d", nmsg.msg.runningtime);
                execl("./process.out", "process.out", runtimeArg, NULL);
                perror("execl failed");
                _exit(1);
            }
            nmsg.msg.pid = pid; // Track the PID
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
                          sumRun += currentProcess->runningtime;
                          sumWait +=( TA-currentProcess->runningtime);
                          sumWTA += WTA;
                          TAs[taIdx]  = TA;
                          WTAs[taIdx] = WTA;
                          taIdx++;
                          free(currentProcess);
                          currentProcess = NULL;
                          completedProcesses++;
                          taIdx++;
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
    FILE *perf = fopen("scheduler.perf", "w");
        if (perf) {
            fprintf(perf, "CPU Util=%.2f%%\n", sumRun / (float)lastClockTime * 100);
            fprintf(perf, "Avg TA=%.2f\n", (sumWait + sumRun) / (float)ProcessesCount);
            fprintf(perf, "Avg WTA=%.2f\n", sumWTA / ProcessesCount);
            fprintf(perf, "Avg Wait=%.2f\n", (float)sumWait / ProcessesCount);
            fclose(perf);
        } else {
            perror("fopen scheduler.perf");
        }

    destroyMinHeap(readyQueue);
    printf("HPF: All %d processes completed\n", completedProcesses);
}
#endif