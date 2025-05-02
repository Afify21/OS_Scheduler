#ifndef HPF_H
#define HPF_H
#include "MinHeap.h"

void runHPF(int ProcessesCount)
{
    MinHeap *readyQueue = createMinHeap(MAX_PROCESSES);
    process *currentProcess = NULL;
    int completedProcesses = 0;

    while (completedProcesses < ProcessesCount)
    {
        int currentTime = getClk();

        // Check for new processes
        struct msgbuff newProcMsg;
        while (msgrcv(SendQueueID, &newProcMsg, sizeof(newProcMsg.msg), 0, IPC_NOWAIT) != -1)
        {
            for (int i = 0; i < NumberOfP; i++)
            {
                if (processList[i].id == newProcMsg.mtype)
                {
                    processList[i].remainingtime = newProcMsg.msg;
                    insertMinHeap_HPF(readyQueue, processList[i]);
                    break;
                }
            }
        }

        // Schedule if CPU is idle
        if (!currentProcess && !isEmpty(readyQueue))
        {
            process p = deleteMinHPF(readyQueue);
            currentProcess = &processList[p.id - 1];
            currentProcess->starttime = currentTime;
            currentProcess->responsetime = currentTime - currentProcess->arrivaltime;

            // Fork the process
            pid_t pid = fork();
            if (pid == 0)
            {
                char rt[10];
                sprintf(rt, "%d", currentProcess->runningtime);
                execl("./process.out", "process.out", rt, NULL);
                exit(1);
            }
            else
            {
                currentProcess->pid = pid;
                logEvent(currentTime, currentProcess->id, "started",
                         currentProcess->arrivaltime, currentProcess->runningtime,
                         currentProcess->remainingtime, currentProcess->waittime);
            }
        }

        // Check if current process finished
        if (currentProcess)
        {
            struct msgbuff updateMsg;
            if (msgrcv(ReceiveQueueID, &updateMsg, sizeof(updateMsg.msg), currentProcess->id, IPC_NOWAIT) != -1)
            {
                currentProcess->remainingtime = updateMsg.msg;

                if (updateMsg.msg == 0)
                {
                    currentProcess->waittime = currentTime - currentProcess->arrivaltime;
                    currentProcess->finishtime = currentTime;
                    logEvent(currentTime, currentProcess->id, "finished",
                             currentProcess->arrivaltime, currentProcess->runningtime,
                             0, currentProcess->waittime);
                    completedProcesses++;
                    currentProcess = NULL;
                }
            }
        }

        usleep(1000);
    }

    destroyMinHeap(readyQueue);
}

#endif