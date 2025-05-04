#ifndef SRTN_H
#define SRTN_H

#include "MinHeap.h"
#include <errno.h>
#include <string.h>


void runSRTN(int ProcessesCount)
{
    float TAs[ProcessesCount];         // Turnaround times
float WTAs[ProcessesCount];        // Weighted turnaround times
int taIdx = 0;
int sumRun = 0;
int sumWait = 0;
float sumWTA = 0.0f;
    printf("SRTN: Starting with %d processes\n", ProcessesCount);

    MinHeap *readyQueue = createMinHeap(MAX_PROCESSES);
    process *currentProcess = NULL;
    int receivedProcesses = 0;
    int completedProcesses = 0;
    int lastClockTime = -1;

    while (completedProcesses < ProcessesCount || !HeapisEmpty(readyQueue) || currentProcess)
    {
        struct msgbuff nmsg;
        ssize_t rec = msgrcv(SendQueueID, &nmsg, sizeof(nmsg.msg), 0, IPC_NOWAIT);

        if (rec != -1)
        {
            printf("Scheduler: Received process %d at time %d\n", nmsg.msg.id, nmsg.msg.arrivaltime);
            nmsg.msg.flag = 0;                        // Initialize flag for new processes
            nmsg.msg.lasttime = nmsg.msg.arrivaltime; // Initialize lasttime
            insertMinHeap_SRTN(readyQueue, nmsg.msg);
            receivedProcesses++;
            logEvent(nmsg.msg.arrivaltime, nmsg.msg.id, "arrived", nmsg.msg.arrivaltime,
                     nmsg.msg.runningtime, nmsg.msg.remainingtime,
                     0, 0, 0.0);
                     
        }

        int currentTime = getClk() - 1;
        if (currentTime != lastClockTime)
        {
            lastClockTime = currentTime;

            // Update current process remaining time
            if (currentProcess)
            {
                currentProcess->remainingtime--;
                if (currentProcess->remainingtime <= 0)
                {
                    int TA = currentTime - currentProcess->arrivaltime;
                    float WTA = (float)TA / currentProcess->runningtime;
                    int total_wait = currentProcess->waittime;
                    printf("SRTN: Process %d completed at time %d\n", currentProcess->id, currentTime);
                    logEvent(currentTime, currentProcess->id, "finished",
                             currentProcess->arrivaltime, currentProcess->runningtime,
                             0, TA-currentProcess->runningtime, TA, WTA);
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

            // Check for preemption
            if (currentProcess && !HeapisEmpty(readyQueue))
            {
                process minProcess = getMin(readyQueue);
                if (minProcess.remainingtime < currentProcess->remainingtime)
                {
                    // Calculate wait time for stop event
                    int wait_duration = currentTime - currentProcess->lasttime;
                    currentProcess->waittime += wait_duration;

                    printf("SRTN: Stopping process %d at time %d\n", currentProcess->id, currentTime);
                    logEvent(currentTime, currentProcess->id, "stopped",
                             currentProcess->arrivaltime, currentProcess->runningtime,
                             currentProcess->remainingtime, wait_duration, 0, 0.0);

                    // Update last stopped time and return to ready queue
                    currentProcess->lasttime = currentTime;
                    insertMinHeap_SRTN(readyQueue, *currentProcess);
                    free(currentProcess);
                    currentProcess = NULL;
                }
            }

            // Schedule new process
            if (!currentProcess && !HeapisEmpty(readyQueue))
            {
                process p = deleteMinSRTN(readyQueue);
                currentProcess = malloc(sizeof(process));
                *currentProcess = p;

                const char *state = "started";
                int wait_duration;

                if (currentProcess->flag)
                {
                    state = "resumed";
                    wait_duration = currentTime - currentProcess->lasttime;
                }
                else
                {
                    state = "started";
                    wait_duration = currentTime - currentProcess->arrivaltime;
                    currentProcess->flag = 1;
                }

                // Update process tracking times
                currentProcess->waittime += wait_duration;
                currentProcess->lasttime = currentTime;

                printf("SRTN: %s process %d at time %d (Remaining: %d)\n",
                       state, currentProcess->id, currentTime, currentProcess->remainingtime);
                logEvent(currentTime, currentProcess->id, state,
                         currentProcess->arrivaltime, currentProcess->runningtime,
                         currentProcess->remainingtime, wait_duration, 0, 0.0);
            }
        }

        usleep(1000);
        
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
    printf("SRTN: All %d processes completed\n", completedProcesses);
}

#endif