#ifndef ROUNDROBIN_H
#define ROUNDROBIN_H

#include "headers.h"
#include "CircularList.h"
#include "Defined_DS.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h> // calloc, free
#include <math.h>   // For sqrt function to calculate standard deviation

// External reference to the global memory allocator
extern BuddyAllocator *memoryAllocator;

// Function to calculate standard deviation
float calculateStdDevRR(float values[], int count, float mean)
{
    float sum_squared_diff = 0.0;
    for (int i = 0; i < count; i++)
    {
        float diff = values[i] - mean;
        sum_squared_diff += diff * diff;
    }
    return sqrt(sum_squared_diff / count);
}

// Internal log helper: append formatted message to scheduler.log
static void log_message(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    FILE *f = fopen("scheduler.log", "a");
    if (f)
    {
        vfprintf(f, fmt, ap);
        fclose(f);
    }
    va_end(ap);
}

/**
 * Round Robin scheduler driven by getClk().
 * @param quantum      Time slice length
 * @param processCount Total number of processes to schedule
 */
void RoundRobin(int quantum, int processCount)
{
    // Clear log and write headers
    FILE *lf = fopen("scheduler.log", "w");
    if (lf)
        fclose(lf);
    int startTime = getClk();
    log_message("# Scheduler start at time %d\n", startTime);
    log_message("# algo=3, quantum=%d\n", quantum);
    log_message("#At time x process y state arr w total z remain y wait k\n");

    // Blocked-time counters per PID (indexed 1..processCount)
    int *waitTimes = calloc(processCount + 1, sizeof(int)); // Creates an array to track how long each process waits
    if (!waitTimes)                                         // initializes all to be 0
    {
        perror("calloc waitTimes");
        return;
    }

    // Print initial memory state
    printf("Initial memory state:\n");
    printMemoryState(memoryAllocator);

    // Ready queue and waiting queue
    struct CircularList *rq = createCircularList();
    struct CircularList *waitingQ = createCircularList(); // Queue for processes waiting for memory
    int remaining = processCount;
    int lastClk = -1; // helps detect when the clock has advanced
    int tickCnt = 0;  // current quantum  progress

    // bn7sb el waited ta w el ta  34an el cpu utilization
    float TA[processCount], WTA[processCount];
    int taIdx = 0, sumRun = 0, sumWait = 0;
    float sumWTA = 0.0f;

    // Message-queue IDs
    int ReadyQ, SendQ, RecvQ;
    DefineKeys(&ReadyQ, &SendQ, &RecvQ);

    while (remaining > 0)
    {
        int clk = getClk();
        if (clk == lastClk)
            continue;
        lastClk = clk;
        int t = clk; // Use the current clock value to start from 1 instead of 0

        // 1) Enqueue arrivals
        struct msgbuff in;
        while (msgrcv(ReadyQ, &in, sizeof(in.msg), 0, IPC_NOWAIT) != -1) // recieves all the arriving processes from the
        // message quueue  and gets the remaining time
        {
            in.msg.flag = 0;
            in.msg.remainingtime = in.msg.runningtime;

            // Allocate memory for the process
            printf("RR: Attempting to allocate %d bytes for process %d\n", in.msg.memsize, in.msg.id);

            // Print memory state before allocation
            printMemoryState(memoryAllocator);

            MemoryBlock *block = allocateMemory(memoryAllocator, in.msg.memsize, in.msg.id);
            if (!block)
            {
                printf("RR: Not enough memory for process %d, adding to waiting queue\n", in.msg.id);

                // Add to waiting queue instead of skipping
                insertAtEnd(waitingQ, in.msg);
            }
            else
            {
                printf("RR: Successfully allocated memory for process %d\n", in.msg.id);

                // Print memory state after allocation
                printMemoryState(memoryAllocator);

                insertAtEnd(rq, in.msg);
            }
        }

        if (isEmpty(rq))
            continue; // heck if the ready queue is empty.
        // If it is, the scheduler just skips this tick — nothing to do.

        // 2) Increment blocked time for all except head
        struct process head;
        getCurrent(rq, &head);
        changeCurrent(rq);
        while (1)
        {
            struct process p;
            getCurrent(rq, &p);
            if (p.id == head.id)
                break;
            waitTimes[p.id]++;
            changeCurrent(rq);
        }

        // 3) Dispatch head of ready queue
        struct process cur;
        getCurrent(rq, &cur);
        printf("[RR] Dispatching P%d at t=%d\n", cur.id, t);
        fflush(stdout); // This line forces any output waiting in the buffer to be immediately written to the screen or output device.

        // Decrement remaining time and count CPU tick
        cur.remainingtime--;
        changeCurrentData(rq, cur);
        sumRun++;
        // Decreases its remainingtime
        // Updates the queue
        // Increases total CPU runtim

        // Log start vs. resume
        if (cur.flag == 0) // first time getting cpu time
        {
            log_message(
                "At time %d process %d started arr %d total %d remain %d wait %d\n",
                t, cur.id, cur.arrivaltime, cur.runningtime,
                cur.remainingtime, waitTimes[cur.id]);
            cur.flag = 1;
            changeCurrentData(rq, cur);
        }
        else
        {
            log_message(
                "At time %d process %d resumed arr %d total %d remain %d wait %d\n",
                t, cur.id, cur.arrivaltime, cur.runningtime,
                cur.remainingtime, waitTimes[cur.id]);
        }
        tickCnt++; // increments the quantum counter to check against it fl akher

        // 4) Finish detection
        if (cur.remainingtime == 0)
        {
            printf("[RR] Process %d finished at t=%d\n", cur.id, t);
            fflush(stdout);
            int turnaround = t - cur.arrivaltime;
            float wta = (float)turnaround / cur.runningtime;
            int blocked = waitTimes[cur.id]; // how long is it waiting in the queue
            log_message(
                "At time %d process %d finished arr %d total %d remain 0 wait %d TA %d WTA %.2f\n",
                t, cur.id, cur.arrivaltime, cur.runningtime,
                blocked, turnaround, wta);
            sumWait += blocked;
            sumWTA += wta;
            TA[taIdx] = turnaround;
            WTA[taIdx] = wta;
            taIdx++;

            // Free memory allocated to the process
            MemoryBlock *block = NULL;

            // Find the memory block for this process
            MemoryBlock *current = memoryAllocator->memory_list;
            while (current)
            {
                if (!current->is_free && current->process_id == cur.id)
                {
                    // Found the block for this process
                    block = current;
                    break;
                }
                current = current->next;
            }

            if (block)
            {
                printf("RR: Freeing memory for process %d\n", cur.id);

                // Print memory state before freeing
                printMemoryState(memoryAllocator);

                freeMemory(memoryAllocator, block, cur.id);

                // Print memory state after freeing
                printMemoryState(memoryAllocator);

                // Check if any waiting processes can now be allocated memory
                if (!isEmpty(waitingQ))
                {
                    struct process waitingProcess;
                    getCurrent(waitingQ, &waitingProcess);

                    // Try to allocate memory for it
                    MemoryBlock *newBlock = allocateMemory(memoryAllocator, waitingProcess.memsize, waitingProcess.id);

                    if (newBlock)
                    {
                        // Successfully allocated memory, move to ready queue
                        printf("RR: Now able to allocate memory for waiting process %d\n", waitingProcess.id);
                        removeCurrent(waitingQ, NULL);   // Remove from waiting queue
                        insertAtEnd(rq, waitingProcess); // Add to ready queue

                        // Print memory state after allocation
                        printMemoryState(memoryAllocator);
                    }
                    else
                    {
                        // Still not enough memory, leave in waiting queue
                        printf("RR: Still not enough memory for waiting process %d\n", waitingProcess.id);
                    }
                }
            }
            else
            {
                printf("RR: Warning - Could not find memory block for process %d\n", cur.id);
            }

            removeCurrent(rq, NULL);
            remaining--; // emoves it from the ready queue.
            // Decreases the number of processes left.
            // Resets tick counter.
            // continue skips the rest of the loop — no need to preempt a process that just finished.

            tickCnt = 0;
            continue; // skip preemption for finished
        }

        // 5) Preempt if quantum expired
        if (tickCnt == quantum)
        {
            tickCnt = 0;
            struct process prev; // Retrieves the process about to be preempted.
            getCurrent(rq, &prev);
            changeCurrent(rq);
            log_message(
                "At time %d process %d stopped arr %d total %d remain %d wait %d\n",
                t, prev.id, prev.arrivaltime, prev.runningtime,
                prev.remainingtime, waitTimes[prev.id]);
        }
    }

    // 6) Write performance metrics
    FILE *perf = fopen("scheduler.perf", "w");
    if (perf)
    {
        // Calculate average WTA
        float avgWTA = sumWTA / processCount;

        // Calculate standard deviation of WTA
        float stdDevWTA = calculateStdDevRR(WTA, taIdx, avgWTA);

        fprintf(perf, "CPU Util=%.2f%%\n", (sumRun / (float)lastClk) * 100);
        fprintf(perf, "Avg TA=%.2f\n", (sumWait + sumRun) / (float)processCount);
        fprintf(perf, "Avg WTA=%.2f\n", avgWTA);
        fprintf(perf, "Std WTA=%.2f\n", stdDevWTA);
        fprintf(perf, "Avg Wait=%.2f\n", (float)sumWait / processCount);
        fclose(perf);
    }

    free(waitTimes);
    destroyList(rq);
    destroyList(waitingQ);
}

#endif // ROUNDROBIN_H
