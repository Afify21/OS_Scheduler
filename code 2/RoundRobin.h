#ifndef ROUNDROBIN_H
#define ROUNDROBIN_H

#include "headers.h"
#include "CircularList.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <stdio.h>


// Internal log helper: append formatted message to scheduler.log
static void logf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    FILE *f = fopen("scheduler.log", "a");
    if (f) {
        vfprintf(f, fmt, ap);
        fclose(f);
    }
    va_end(ap);
}

/**
 * Round Robin scheduler driven by getClk(), no sync API.
 * @param quantum      Time slice length
 * @param processCount Total number of processes to schedule
 */
void RoundRobin(int quantum, int processCount) {
    // Header in log (simulate start at t=0)
    logf("--- RR start: quantum=%d, total=%d at t=%d ---\n",
         quantum, processCount, getClk() - 1);

    // Shared-memory flags (optional for external observers)
    key_t kRun = ftok("keys/Guirunningman", 'A');
    int shmR = shmget(kRun, sizeof(int), IPC_CREAT | 0644);
    int *runningPid = shmat(shmR, NULL, 0);
    *runningPid = -1;

    key_t kDead = ftok("keys/Guideadman", 'A');
    int shmD = shmget(kDead, sizeof(int), IPC_CREAT | 0644);
    int *deadPid = shmat(shmD, NULL, 0);
    *deadPid = -1;

    // Message-queue IDs
    int ReadyQ, SendQ, RecvQ;
    DefineKeys(&ReadyQ, &SendQ, &RecvQ);

    // Ready queue as a circular list
    struct CircularList *rq = createCircularList();
    int remaining = processCount;
    int lastClk = -1;
    int tickCnt = 0;

    // Performance metrics
    float TA[processCount];         // Turnaround times
    float WTA[processCount];        // Weighted turnaround times
    int taIdx = 0;
    int sumRun = 0;
    int sumWait = 0;
    float sumWTA = 0.0f;

    // Main scheduling loop: iterate once per clock tick
    while (remaining > 0) {
        int clk = getClk();
        if (clk == lastClk) continue;
        lastClk = clk;
        int t = clk - 1;  // adjust time to start at 0

        // 1) Enqueue all new arrivals that arrived at or before this tick
        struct msgbuff buf;
        while (msgrcv(ReadyQ, &buf, sizeof(buf.msg), 0, IPC_NOWAIT) != -1) {
            buf.msg.flag = 0;  // mark as not yet started
            pid_t pid = fork();
            if (pid == 0) {
                char arg[16];
                sprintf(arg, "%d", buf.msg.runningtime);
                execl("./process.out", "process.out", arg, NULL);
                _exit(1);
            }
            buf.msg.pid = pid;
            insertAtEnd(rq, buf.msg);
            logf("Enqueued P%d at t=%d\n", buf.msg.id, t);
        }

        // 2) If someone ran in the previous tick, collect its update
        if (!isEmpty(rq)) {
            struct msgbuff rbuf;
            if (msgrcv(RecvQ, &rbuf, sizeof(rbuf.msg), 0, IPC_NOWAIT) != -1) {
                struct process cur;
                getCurrent(rq, &cur);
                cur.remainingtime = rbuf.msg.remainingtime;
                changeCurrentData(rq, cur);

                if (cur.remainingtime == 0) {
                    // Compute Turnaround Time (TA) and Weighted Turnaround (WTA)
                    int turnaround = t - cur.arrivaltime;
                    int waitTime = turnaround - cur.runningtime;
                    float wta = (float)turnaround / cur.runningtime;  // TA / burst time

                    logf("P%d finished at t=%d, wait=%d, TA=%d, WTA=%.2f\n",
                         cur.id, t, waitTime, turnaround, wta);

                    // Accumulate metrics
                    sumRun += cur.runningtime;
                    sumWait += waitTime;
                    sumWTA += wta;
                    TA[taIdx]  = turnaround;
                    WTA[taIdx] = wta;
                    taIdx++;

                    // Remove finished process and move to next
                    removeCurrent(rq, NULL);
                    remaining--;
                    tickCnt = 0;
                    wait(NULL);  // reap finished child
                }
            }

            // 3) Preempt if time slice expired
            if (tickCnt == quantum) {
                tickCnt = 0;
                struct process prev;
                getCurrent(rq, &prev);
                changeCurrent(rq);
                logf("P%d preempted at t=%d\n", prev.id, t);
            }

            // 4) Dispatch the current process
            if (!isEmpty(rq)) {
                struct process nxt;
                getCurrent(rq, &nxt);
                printf("[RR] Dispatching P%d at t=%d\n", nxt.id, t);
                fflush(stdout);

                struct msgbuff smb;
                smb.mtype = nxt.pid;
                smb.msg = nxt;
                if (msgsnd(SendQ, &smb, sizeof(smb.msg), 0) == -1) {
                    perror("msgsnd SendQ");
                }

                // Log start vs. resume
                if (nxt.runningtime == nxt.remainingtime) {
                    logf("P%d started at t=%d\n", nxt.id, t);
                } else {
                    logf("P%d resumed at t=%d\n", nxt.id, t);
                }
                tickCnt++;
            }
        }
    }

    // 5) Write performance metrics: CPU Util, Avg TA, Avg WTA, Avg Wait
    {
        FILE *perf = fopen("scheduler.perf", "w");
        if (perf) {
            fprintf(perf, "CPU Util=%.2f%%", sumRun / (float)lastClk * 100);
            fprintf(perf, "Avg TA=%.2f", (sumWait + sumRun) / (float)processCount);
            fprintf(perf, "Avg WTA=%.2f", sumWTA / processCount);
            fprintf(perf, "Avg Wait=%.2f", (float)sumWait / processCount);
            fclose(perf);
        } else {
            perror("fopen scheduler.perf");
        }
    }

    // Cleanup shared memory and list
    shmdt(runningPid);
    shmdt(deadPid);
    destroyList(rq);
}

#endif // ROUNDROBIN_H
