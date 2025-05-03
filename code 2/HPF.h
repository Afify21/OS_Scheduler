#ifndef HPF_H
#define HPF_H
#include "MinHeap.h"
#include <errno.h>
static ssize_t rec;         // NOLINT
void runHPF(int ProcessesCount) {
    printf("HPF: Starting with %d processes\n", ProcessesCount);

    MinHeap *readyQueue = createMinHeap(MAX_PROCESSES);
    process *currentProcess = NULL;
    int receivedProcesses = 0;
    int lastClockTime = -1;

    while (receivedProcesses < ProcessesCount || !HeapisEmpty(readyQueue) || currentProcess) {
        struct msgbuff nmsg;
        ssize_t rec = msgrcv(SendQueueID, &nmsg, sizeof(nmsg.msg), 0, IPC_NOWAIT);
        if (rec == -1) {
            if (errno != ENOMSG) perror("msgrcv failed"); // ENOMSG is expected when the queue is empty
        } else {
            printf("Scheduler: Received process  %d at time %d\n", nmsg.msg.id, getClk());
            insertMinHeap_HPF(readyQueue, nmsg.msg);
            receivedProcesses++;
        }

        int currentTime = getClk();
        if (currentTime != lastClockTime) {
            lastClockTime = currentTime;

            // Schedule a new process if none is running
            if (!currentProcess && !HeapisEmpty(readyQueue)) {
                process p = deleteMinHPF(readyQueue);
                currentProcess = &p;

                // Fork and execute the process
                pid_t pid = fork();
                if (pid == 0) {
                    execl("./process.out", "process.out", currentProcess->runningtime, NULL);
                    exit(EXIT_FAILURE);
                } else {
                    currentProcess->pid = pid;
                    printf("HPF: Started process %d at time %d\n", currentProcess->id, currentTime);
                }
            }

            // Check if current process has finished
            if (currentProcess) {
                int status;
                pid_t result = waitpid(currentProcess->pid, &status, WNOHANG);
                if (result > 0) {
                    printf("HPF: Process %d completed at time %d\n", currentProcess->id, getClk()+currentProcess->runningtime);
                    currentProcess = NULL;
                }
            }
        }
        usleep(1000); // Avoid busy waiting
    }

    destroyMinHeap(readyQueue);
    printf("HPF: All processes completed\n");
}
#endif
