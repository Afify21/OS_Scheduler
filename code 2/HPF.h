#ifndef HPF_H
#define HPF_H
#include "MinHeap.h"

void runHPF(int ProcessesCount)
{
    printf("HPF: Starting with %d processes\n", ProcessesCount);

    MinHeap *readyQueue = createMinHeap(MAX_PROCESSES);
    process *currentProcess = NULL;
    int completedProcesses = 0;
    int lastClockTime = -1;

    while (completedProcesses < ProcessesCount)
    {
        int currentTime = getClk();

        if (currentTime != lastClockTime)
        {
            lastClockTime = currentTime;

            // Check for new processes
            struct msgbuff newProcMsg;
            while (msgrcv(SendQueueID, &newProcMsg, sizeof(newProcMsg.msg), 0, IPC_NOWAIT) != -1)
            {
                int pid = newProcMsg.mtype;
                int remainingTime = newProcMsg.msg;

                // Update process details from message
                int idx = pid - 1;
                if (idx >= 0 && idx < ProcessesCount)
                {
                    processList[idx].remainingtime = remainingTime;
                    processList[idx].flag = 1;

                    // Insert into ready queue
                    insertMinHeap_HPF(readyQueue, processList[idx]);
                    printf("HPF: Enqueued process %d (prio %d)\n",
                           processList[idx].id, processList[idx].priority);
                }
            }

            // Schedule new process if needed
            if (!currentProcess && !isEmpty(readyQueue))
            {
                process p = deleteMinHPF(readyQueue);
                int idx = p.id - 1;

                currentProcess = &processList[idx];
                currentProcess->starttime = currentTime;

                // Fork and execute
                pid_t childPid = fork();
                if (childPid == 0)
                {
                    // Child process
                    execl("./process.out", "process.out",
                          currentProcess->runningtime, NULL);
                    exit(1);
                }
                else
                {
                    // Parent process
                    currentProcess->pid = childPid;
                    printf("HPF: Started %d at %d (prio %d)\n",
                           currentProcess->id, currentTime,
                           currentProcess->priority);
                }
            }

            // Check running process status
            if (currentProcess)
            {
                int status;
                pid_t result = waitpid(currentProcess->pid, &status, WNOHANG);

                if (result > 0)
                {
                    // Process completed
                    printf("HPF: Completed %d at %d\n",
                           currentProcess->id, currentTime);
                    completedProcesses++;
                    currentProcess = NULL;
                }
                else
                {
                    // Update remaining time
                    struct msgbuff updateMsg;
                    if (msgrcv(ReceiveQueueID, &updateMsg,
                               sizeof(updateMsg.msg), currentProcess->id, IPC_NOWAIT) != -1)
                    {
                        currentProcess->remainingtime = updateMsg.msg;
                    }
                }
            }
        }
        usleep(1000); // Prevent CPU hogging
    }

    destroyMinHeap(readyQueue);
    printf("HPF: All processes completed\n");
}

#endif