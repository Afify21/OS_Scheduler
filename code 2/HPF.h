#ifndef HPF_H
#define HPF_H
#include "MinHeap.h"
#include "Defined_DS.h"
#include <errno.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <math.h> // For sqrt function to calculate standard deviation

// External reference to the global memory allocator
extern BuddyAllocator *memoryAllocator;

// Function to calculate standard deviation
float calculateStdDev(float values[], int count, float mean)
{
    float sum_squared_diff = 0.0;
    for (int i = 0; i < count; i++)
    {
        float diff = values[i] - mean;
        sum_squared_diff += diff * diff;
    }
    return sqrt(sum_squared_diff / count);
}

void runHPF(int ProcessesCount)
{
    printf("HPF: Starting with %d processes\n", ProcessesCount);

    // Print initial memory state
    printf("Initial memory state:\n");
    printMemoryState(memoryAllocator);

    MinHeap *readyQueue = createMinHeap(MAX_PROCESSES);
    MinHeap *waitingQueue = createMinHeap(MAX_PROCESSES); // Queue for processes waiting for memory
    process *currentProcess = NULL;
    int receivedProcesses = 0;
    int completedProcesses = 0;
    int lastClockTime = -1;
    float TAs[ProcessesCount];  // Turnaround times
    float WTAs[ProcessesCount]; // Weighted turnaround times
    int taIdx = 0;
    int sumRun = 0;
    int sumWait = 0;
    float sumWTA = 0.0f;

    while (completedProcesses < ProcessesCount || !HeapisEmpty(readyQueue) || currentProcess)
    {
        // Get the current simulation time once at the start of each loop

        // Check for new processes arriving from process_generator
        struct msgbuff nmsg;
        ssize_t rec = msgrcv(SendQueueID, &nmsg, sizeof(nmsg.msg), 0, IPC_NOWAIT);

        if (rec != -1)
        {
            printf("HPF: Received process %d at time %d\n", nmsg.msg.id, nmsg.msg.arrivaltime);
            // Fork the process
            pid_t pid = fork();
            if (pid == 0)
            {
                char runtimeArg[16];
                sprintf(runtimeArg, "%d", nmsg.msg.runningtime);
                execl("./process.out", "process.out", runtimeArg, NULL);
                perror("execl failed");
                _exit(1);
            }
            nmsg.msg.pid = pid; // Store PID for messaging

            // Check if this process is already in the waiting queue
            bool already_waiting = false;
            if (!HeapisEmpty(waitingQueue))
            {
                // Create a temporary copy of the waiting queue to check
                MinHeap *tempQueue = createMinHeap(MAX_PROCESSES);

                // Check each process in the waiting queue
                while (!HeapisEmpty(waitingQueue))
                {
                    process temp = getMin(waitingQueue);
                    if (temp.id == nmsg.msg.id)
                    {
                        already_waiting = true;
                    }
                    insertMinHeap_HPF(tempQueue, temp);
                    deleteMinHPF(waitingQueue);
                }

                // Restore the waiting queue
                while (!HeapisEmpty(tempQueue))
                {
                    process temp = getMin(tempQueue);
                    insertMinHeap_HPF(waitingQueue, temp);
                    deleteMinHPF(tempQueue);
                }

                destroyMinHeap(tempQueue);
            }

            if (already_waiting)
            {
                printf("HPF: Process %d is already in the waiting queue\n", nmsg.msg.id);
                continue;
            }

            // Allocate memory for the process
            printf("HPF: Attempting to allocate %d bytes for process %d\n", nmsg.msg.memsize, nmsg.msg.id);

            // Print memory state before allocation
            printMemoryState(memoryAllocator);

            MemoryBlock *block = allocateMemory(memoryAllocator, nmsg.msg.memsize, nmsg.msg.id);
            if (!block)
            {
                printf("HPF: Not enough memory for process %d, adding to waiting queue\n", nmsg.msg.id);

                // Add to waiting queue instead of killing
                insertMinHeap_HPF(waitingQueue, nmsg.msg);

                // Log that the process is waiting for memory
                logEvent(nmsg.msg.arrivaltime, nmsg.msg.id, "arrived", nmsg.msg.arrivaltime,
                         nmsg.msg.runningtime, nmsg.msg.runningtime, 0, 0, 0);

                receivedProcesses++;
            }
            else
            {
                printf("HPF: Successfully allocated memory for process %d\n", nmsg.msg.id);

                // Print memory state after allocation
                printMemoryState(memoryAllocator);

                insertMinHeap_HPF(readyQueue, nmsg.msg);
                receivedProcesses++;
                logEvent(nmsg.msg.arrivaltime, nmsg.msg.id, "arrived", nmsg.msg.arrivaltime,
                         nmsg.msg.runningtime, nmsg.msg.runningtime, 0, 0, 0);
            }
        }
        int currentTime = getClk() - 1;

        // Check for process updates from running process
        struct msgbuff rbuf;
        if (msgrcv(ReceiveQueueID, &rbuf, sizeof(rbuf.msg), 0, IPC_NOWAIT) != -1)
        {
            if (currentProcess && currentProcess->pid == rbuf.msg.pid)
            {
                // Get the exact clock time from the message if available
                int updateTime = (rbuf.msg.lasttime > 0) ? rbuf.msg.lasttime : currentTime;

                // Update the remaining time based on the process report
                currentProcess->remainingtime = rbuf.msg.remainingtime;
                printf("HPF: Received update from process %d at time %d: remaining time = %d\n",
                       currentProcess->id, updateTime, currentProcess->remainingtime);

                if (currentProcess->remainingtime <= 0)
                {
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

                    // Free memory allocated to the process
                    MemoryBlock *block = NULL;

                    // Find the memory block for this process
                    printf("HPF: Searching for memory block for process %d\n", currentProcess->id);
                    MemoryBlock *current = memoryAllocator->memory_list;
                    int blockIndex = 0;
                    while (current)
                    {
                        printf("HPF: Checking block %d: addr=%d, size=%d, free=%d, pid=%d\n",
                               blockIndex++, current->start_address, current->size,
                               current->is_free, current->process_id);

                        if (!current->is_free && current->process_id == currentProcess->id)
                        {
                            // Found the block for this process
                            printf("HPF: Found memory block for process %d at address %d\n",
                                   currentProcess->id, current->start_address);
                            block = current;
                            break;
                        }
                        current = current->next;
                    }

                    if (block)
                    {
                        printf("HPF: Freeing memory for process %d\n", currentProcess->id);

                        // Print memory state before freeing
                        printMemoryState(memoryAllocator);

                        freeMemory(memoryAllocator, block, currentProcess->id);

                        // Print memory state after freeing
                        printMemoryState(memoryAllocator);

                        // Check if any waiting processes can now be allocated memory
                        printf("HPF: Checking waiting queue for processes that can now be allocated memory\n");

                        if (!HeapisEmpty(waitingQueue))
                        {
                            printf("HPF: Waiting queue is not empty\n");
                        }
                        else
                        {
                            printf("HPF: Waiting queue is empty\n");
                        }

                        while (!HeapisEmpty(waitingQueue))
                        {
                            // Get the highest priority waiting process
                            process waitingProcess = getMin(waitingQueue);
                            printf("HPF: Trying to allocate memory for waiting process %d (size: %d)\n",
                                   waitingProcess.id, waitingProcess.memsize);

                            // Try to allocate memory for it
                            MemoryBlock *newBlock = allocateMemory(memoryAllocator, waitingProcess.memsize, waitingProcess.id);

                            if (newBlock)
                            {
                                // Successfully allocated memory, move to ready queue
                                printf("HPF: Now able to allocate memory for waiting process %d\n", waitingProcess.id);
                                deleteMinHPF(waitingQueue);                    // Remove from waiting queue
                                insertMinHeap_HPF(readyQueue, waitingProcess); // Add to ready queue

                                // Print memory state after allocation
                                printMemoryState(memoryAllocator);

                                // Update the process in the ready queue
                                printf("HPF: Process %d moved from waiting queue to ready queue\n", waitingProcess.id);
                            }
                            else
                            {
                                // Still not enough memory, leave in waiting queue
                                printf("HPF: Still not enough memory for waiting process %d\n", waitingProcess.id);
                                break;
                            }
                        }
                    }
                    else
                    {
                        printf("HPF: Warning - Could not find memory block for process %d\n", currentProcess->id);
                    }

                    // Cleanup and mark as completed
                    free(currentProcess);
                    currentProcess = NULL;
                    completedProcesses++;
                    waitpid(rbuf.msg.pid, NULL, 0); // Reap child
                }
            }
        }

        // Check if clock has advanced
        if (currentTime != lastClockTime)
        {
            lastClockTime = currentTime;

            // Schedule new process if CPU is free
            if (!currentProcess && !HeapisEmpty(readyQueue))
            {
                process p = deleteMinHPF(readyQueue);
                currentProcess = malloc(sizeof(process));
                *currentProcess = p;

                // Send message to process to start
                struct msgbuff smb;
                smb.mtype = currentProcess->pid;
                smb.msg = *currentProcess;
                if (msgsnd(SendQueueID, &smb, sizeof(smb.msg), 0) == -1)
                {
                    perror("HPF: msgsnd failed");
                }
                else
                {
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
    if (perf)
    {
        // Calculate average WTA
        float avgWTA = sumWTA / ProcessesCount;

        // Calculate standard deviation of WTA
        float stdDevWTA = calculateStdDev(WTAs, taIdx, avgWTA);

        fprintf(perf, "CPU Util=%.2f%%\n", sumRun / (float)lastClockTime * 100);
        fprintf(perf, "Avg TA=%.2f\n", (sumWait + sumRun) / (float)ProcessesCount);
        fprintf(perf, "Avg WTA=%.2f\n", avgWTA);
        fprintf(perf, "Std WTA=%.2f\n", stdDevWTA);
        fprintf(perf, "Avg Wait=%.2f\n", (float)sumWait / ProcessesCount);
        fclose(perf);
    }
    else
    {
        perror("fopen scheduler.perf");
    }

    destroyMinHeap(readyQueue);
    destroyMinHeap(waitingQueue);
    printf("HPF: All %d processes completed\n", completedProcesses);
}
#endif