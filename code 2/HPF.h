#ifndef HPF_H
#define HPF_H
#include "MinHeap.h"
#include <errno.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/wait.h>

void runHPF(int ProcessesCount) {
    printf("HPF: Starting with %d processes\n", ProcessesCount);

    MinHeap *readyQueue = createMinHeap(MAX_PROCESSES);
    process *currentProcess = NULL;
    int receivedProcesses = 0;
    int completedProcesses = 0;
    int lastClockTime = -1;
    float TAs[ProcessesCount];         // Turnaround times
    float WTAs[ProcessesCount];        // Weighted turnaround times
    int taIdx = 0;
    int sumRun = 0;
    int sumWait = 0;
    float sumWTA = 0.0f;

    while (completedProcesses < ProcessesCount || !HeapisEmpty(readyQueue) || currentProcess) {
        // Get the current simulation time once at the start of each loop
        
        // Check for new processes arriving from process_generator
        struct msgbuff nmsg;
        ssize_t rec = msgrcv(SendQueueID, &nmsg, sizeof(nmsg.msg), 0, IPC_NOWAIT);

        if (rec != -1) {
            printf("HPF: Received process %d at time %d\n", nmsg.msg.id, nmsg.msg.arrivaltime);
            // Fork the process
            pid_t pid = fork();
            if (pid == 0) {
                char runtimeArg[16];
                sprintf(runtimeArg, "%d", nmsg.msg.runningtime);
                execl("./process.out", "process.out", runtimeArg, NULL);
                perror("execl failed");
                _exit(1);
            }
            nmsg.msg.pid = pid; // Store PID for messaging
            insertMinHeap_HPF(readyQueue, nmsg.msg);
            receivedProcesses++;
            logEvent(nmsg.msg.arrivaltime, nmsg.msg.id, "arrived", nmsg.msg.arrivaltime, 
                     nmsg.msg.runningtime, nmsg.msg.runningtime, 0, 0, 0);
        }
        int currentTime = getClk()-1;

        // Check for process updates from running process
        struct msgbuff rbuf;
        if (msgrcv(ReceiveQueueID, &rbuf, sizeof(rbuf.msg), 0, IPC_NOWAIT) != -1) {
            if (currentProcess && currentProcess->pid == rbuf.msg.pid) {
                // Get the exact clock time from the message if available
                int updateTime = (rbuf.msg.lasttime > 0) ? rbuf.msg.lasttime : currentTime;
                
                // Update the remaining time based on the process report
                currentProcess->remainingtime = rbuf.msg.remainingtime;
                printf("HPF: Received update from process %d at time %d: remaining time = %d\n", 
                       currentProcess->id, updateTime, currentProcess->remainingtime);
                
                if (currentProcess->remainingtime <= 0) {
                    // Process has completed
                    int TA = updateTime - currentProcess->arrivaltime;
                    float WTA = (float)TA / currentProcess->runningtime;
                    printf("HPF: Process %d completed at time %d\n", currentProcess->id, updateTime);
                    
                    // Log the completion event with the exact finish time
                    logEvent(updateTime, currentProcess->id, "finished", 
                            currentProcess->arrivaltime, currentProcess->runningtime, 
                            0, TA - currentProcess->runningtime, TA, WTA);
                    
                    // Update statistics
                    sumRun += currentProcess->runningtime;
                    sumWait += (TA - currentProcess->runningtime);
                    sumWTA += WTA;
                    TAs[taIdx] = TA;
                    WTAs[taIdx] = WTA;
                    taIdx++;
                    
                    // Cleanup and mark as completed
                    free(currentProcess);
                    currentProcess = NULL;
                    completedProcesses++;
                    waitpid(rbuf.msg.pid, NULL, 0); // Reap child
                }
            }
        }

        // Check if clock has advanced
        if (currentTime != lastClockTime) {
            lastClockTime = currentTime;

            // Schedule new process if CPU is free
            if (!currentProcess && !HeapisEmpty(readyQueue)) {
                process p = deleteMinHPF(readyQueue);
                currentProcess = malloc(sizeof(process));
                *currentProcess = p;

                // Send message to process to start
                struct msgbuff smb;
                smb.mtype = currentProcess->pid;
                smb.msg = *currentProcess;
                if (msgsnd(SendQueueID, &smb, sizeof(smb.msg), 0) == -1) {
                    perror("HPF: msgsnd failed");
                } else {
                    printf("HPF: Sent run signal to process %d at time %d\n", 
                           currentProcess->id, currentTime);
                }

                // Log process start
                int waitTime = currentTime - currentProcess->arrivaltime;
                logEvent(currentTime, currentProcess->id, "started", 
                        currentProcess->arrivaltime, currentProcess->runningtime, 
                        currentProcess->remainingtime, waitTime, 0, 0);
                
                printf("HPF: Started process %d (priority %d) at time %d\n", 
                       currentProcess->id, currentProcess->priority, currentTime);
            }
        }
        
        usleep(1000); // Short sleep to reduce CPU usage
    }

    // Write performance metrics
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