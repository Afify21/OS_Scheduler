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

void runHPF(int ProcessesCount) {
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

            // Try to allocate memory for the process
            printf("HPF: Attempting to allocate %d bytes for process %d\n", nmsg.msg.memsize, nmsg.msg.id);
            printMemoryState(memoryAllocator);
            MemoryBlock *block = allocateMemory(memoryAllocator, nmsg.msg.memsize, nmsg.msg.id);
            if (!block) {
                printf("HPF: Not enough memory for process %d, adding to waiting queue\n", nmsg.msg.id);
                insertMinHeap_HPF(waitingQueue, nmsg.msg);
                // logEvent(nmsg.msg.arrivaltime, nmsg.msg.id, "arrived", nmsg.msg.arrivaltime, 
                //          nmsg.msg.runningtime, nmsg.msg.runningtime, 0, 0, 0);
                // receivedProcesses++;
            } else {
                printf("HPF: Successfully allocated memory for process %d\n", nmsg.msg.id);
                printMemoryState(memoryAllocator);
                insertMinHeap_HPF(readyQueue, nmsg.msg);
                // logEvent(nmsg.msg.arrivaltime, nmsg.msg.id, "arrived", nmsg.msg.arrivaltime, 
                //          nmsg.msg.runningtime, nmsg.msg.runningtime, 0, 0, 0);
                // receivedProcesses++;
            }
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
                    sumWait += (TA - currentProcess->runningtime);
                    sumWTA += WTA;
                    TAs[taIdx] = TA;
                    WTAs[taIdx] = WTA;
                    taIdx++;

                    // Free memory allocated to the process
                    MemoryBlock *block = NULL;
                    MemoryBlock *current = memoryAllocator->memory_list;
                    while (current) {
                        if (!current->is_free && current->process_id == currentProcess->id) {
                            block = current;
                            break;
                        }
                        current = current->next;
                    }

                    if (block) {
                        printf("HPF: Freeing memory for process %d\n", currentProcess->id);
                        printMemoryState(memoryAllocator);
                        freeMemory(memoryAllocator, block, currentProcess->id);
                        printMemoryState(memoryAllocator);

                        // Check if any waiting processes can now be allocated memory
                        while (!HeapisEmpty(waitingQueue)) {
                            process waitingProcess = getMin(waitingQueue);
                            MemoryBlock *newBlock = allocateMemory(memoryAllocator, waitingProcess.memsize, waitingProcess.id);
                            if (newBlock) {
                                printf("HPF: Now able to allocate memory for waiting process %d\n", waitingProcess.id);
                                deleteMinHPF(waitingQueue);
                                insertMinHeap_HPF(readyQueue, waitingProcess);
                                printMemoryState(memoryAllocator);
                            } else {
                                break;
                            }
                        }
                    }

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

    FILE *perf = fopen("scheduler.perf", "w");
    if (perf) {
        float avgWTA = sumWTA / ProcessesCount;
        float stdDevWTA = calculateStdDev(WTAs, taIdx, avgWTA);
        fprintf(perf, "CPU Util=%.2f%%\n", sumRun / (float)lastClockTime * 100);
        fprintf(perf, "Avg TA=%.2f\n", (sumWait + sumRun) / (float)ProcessesCount);
        fprintf(perf, "Avg WTA=%.2f\n", avgWTA);
        fprintf(perf, "Std WTA=%.2f\n", stdDevWTA);
        fprintf(perf, "Avg Wait=%.2f\n", (float)sumWait / ProcessesCount);
        fclose(perf);
    } else {
        perror("fopen scheduler.perf");
    }

    destroyMinHeap(readyQueue);
    destroyMinHeap(waitingQueue);
    printf("HPF: All %d processes completed\n", completedProcesses);
}

#endif