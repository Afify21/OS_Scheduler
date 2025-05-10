#ifndef SRTN_H
#define SRTN_H

#include "MinHeap.h"
#include "Defined_DS.h"
#include <errno.h>
#include <string.h>
#include <math.h> // For sqrt function to calculate standard deviation

// External reference to the global memory allocator
extern BuddyAllocator *memoryAllocator;

// Function to calculate standard deviation
float calculateStdDevSRTN(float values[], int count, float mean)
{
    float sum_squared_diff = 0.0;
    for (int i = 0; i < count; i++)
    {
        float diff = values[i] - mean;
        sum_squared_diff += diff * diff;
    }
    return sqrt(sum_squared_diff / count);
}

void runSRTN(int ProcessesCount)
{
    float TAs[ProcessesCount];  // Turnaround times
    float WTAs[ProcessesCount]; // Weighted turnaround times
    int taIdx = 0;
    int sumRun = 0;
    int sumWait = 0;
    float sumWTA = 0.0f;
    printf("SRTN: Starting with %d processes\n", ProcessesCount);

    // Print initial memory state
    printf("Initial memory state:\n");
    printMemoryState(memoryAllocator);

    MinHeap *readyQueue = createMinHeap(MAX_PROCESSES);
    MinHeap *waitingQueue = createMinHeap(MAX_PROCESSES); // Queue for processes waiting for memory
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
            // Fork and execute process.out
            pid_t pid = fork();
            if (pid == 0)
            {
                char runtimeArg[16];
                sprintf(runtimeArg, "%d", nmsg.msg.runningtime);
                execl("./process.out", "process.out", runtimeArg, NULL);
                perror("execl failed");
                _exit(1);
            }
            nmsg.msg.pid = pid; // Track the PID
            nmsg.msg.flag = 0;
            nmsg.msg.lasttime = nmsg.msg.arrivaltime;

            // Allocate memory for the process
            printf("SRTN: Attempting to allocate %d bytes for process %d\n", nmsg.msg.memsize, nmsg.msg.id);

            // Print memory state before allocation
            printMemoryState(memoryAllocator);

            MemoryBlock *block = allocateMemory(memoryAllocator, nmsg.msg.memsize, nmsg.msg.id);
            if (!block)
            {
                printf("SRTN: Not enough memory for process %d, adding to waiting queue\n", nmsg.msg.id);

                // Add to waiting queue instead of killing
                insertMinHeap_SRTN(waitingQueue, nmsg.msg);

                // Log that the process is waiting for memory
                logEvent(nmsg.msg.arrivaltime, nmsg.msg.id, "arrived", nmsg.msg.arrivaltime,
                         nmsg.msg.runningtime, nmsg.msg.remainingtime,
                         0, 0, 0.0);

                receivedProcesses++;
            }
            else
            {
                printf("SRTN: Successfully allocated memory for process %d\n", nmsg.msg.id);

                // Print memory state after allocation
                printMemoryState(memoryAllocator);

                insertMinHeap_SRTN(readyQueue, nmsg.msg);
                receivedProcesses++;
                logEvent(nmsg.msg.arrivaltime, nmsg.msg.id, "arrived", nmsg.msg.arrivaltime,
                         nmsg.msg.runningtime, nmsg.msg.remainingtime,
                         0, 0, 0.0);
            }
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
                             0, TA - currentProcess->runningtime, TA, WTA);
                    sumRun += currentProcess->runningtime;
                    sumWait += (TA - currentProcess->runningtime);
                    sumWTA += WTA;
                    TAs[taIdx] = TA;
                    TAs[taIdx] = TA;
                    WTAs[taIdx] = WTA;
                    taIdx++;

                    // Free memory allocated to the process
                    MemoryBlock *block = NULL;

                    // Find the memory block for this process
                    MemoryBlock *current = memoryAllocator->memory_list;
                    while (current)
                    {
                        if (!current->is_free && current->process_id == currentProcess->id)
                        {
                            // Found the block for this process
                            block = current;
                            break;
                        }
                        current = current->next;
                    }

                    if (block)
                    {
                        printf("SRTN: Freeing memory for process %d\n", currentProcess->id);

                        // Print memory state before freeing
                        printMemoryState(memoryAllocator);

                        freeMemory(memoryAllocator, block, currentProcess->id);

                        // Print memory state after freeing
                        printMemoryState(memoryAllocator);

                        // Check if any waiting processes can now be allocated memory
                        while (!HeapisEmpty(waitingQueue))
                        {
                            // Get the shortest remaining time waiting process
                            process waitingProcess = getMin(waitingQueue);

                            // Try to allocate memory for it
                            MemoryBlock *newBlock = allocateMemory(memoryAllocator, waitingProcess.memsize, waitingProcess.id);

                            if (newBlock)
                            {
                                // Successfully allocated memory, move to ready queue
                                printf("SRTN: Now able to allocate memory for waiting process %d\n", waitingProcess.id);
                                deleteMinSRTN(waitingQueue);                    // Remove from waiting queue
                                insertMinHeap_SRTN(readyQueue, waitingProcess); // Add to ready queue

                                // Print memory state after allocation
                                printMemoryState(memoryAllocator);
                            }
                            else
                            {
                                // Still not enough memory, leave in waiting queue
                                printf("SRTN: Still not enough memory for waiting process %d\n", waitingProcess.id);
                                break;
                            }
                        }
                    }
                    else
                    {
                        printf("SRTN: Warning - Could not find memory block for process %d\n", currentProcess->id);
                    }

                    free(currentProcess);
                    currentProcess = NULL;
                    completedProcesses++;
                    // Remove duplicate taIdx++ that was here
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
    if (perf)
    {
        // Calculate average WTA
        float avgWTA = sumWTA / ProcessesCount;

        // Calculate standard deviation of WTA
        float stdDevWTA = calculateStdDevSRTN(WTAs, taIdx, avgWTA);

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
    printf("SRTN: All %d processes completed\n", completedProcesses);
}

#endif