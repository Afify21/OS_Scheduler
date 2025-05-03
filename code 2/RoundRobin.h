// okay what do we need?
// el arrival time
// allocation 34an el C m7tag memory
// picking up the next ready process from the circular list,give it a quantum
// updating the remaining time using proccess.c sneding it back to the schedular
//----------------tb what to do?-------------
// awl 7aga mfrod te7sal howa eny lma 7aga t arrive a3mlha enqueue using function insertatend
// tany haga if i either finished el remaining time aw aw el quantum b zero el mfrod a3ml remove current
// 34an hwa quantums f ana 3ayz a rotate 3la el list  using change current to the next one
// peek at the current job using get current
// me7tagen array of the arrived processes el lesa madakhaletsh
// 3ayzen 7aga t track el time used , wait times , weighted turnaround time , arrival blah blah blah

#ifndef ROUNDROBIN_H
#define ROUNDROBIN_H
#include "headers.h"
#include "CircularList.h"
#include <math.h>
#include <time.h>

void clearLogFileRR() // basically this function clears the log file 34an el terminal ykon nedef w mntlaghbatash
{
    FILE *filePointer;
    filePointer = fopen("scheduler.log", "w"); // opens the file in write mode which clears the file
    if (filePointer == NULL)
    {
        printf("Unable to open scheduler.log.\n");
        return;
    }
    fclose(filePointer); // then closes the file
}

// parameters are procces el ana feha w pointer to the running processes to the shared memory slots
void LogStartedRR(struct process proc, int *shared) // logs when a process starts or resumes 34an tdyk el data
{
    int clock = getClk();
    FILE *filePointer;
    filePointer = fopen("scheduler.log", "a"); // a means append if doesnt exist it creates it
    *shared = proc.id;                         // this is the id of the process that is running
    if (filePointer == NULL)
    {
        printf("Unable to open scheduler.log.\n");
        return;
    }
    if (proc.remainingtime != proc.runningtime) // proccess is resuming
    {                                           // wait dispatch_time – arrival_time – time_already_run
        fprintf(filePointer, "At time %d, process %d resumed. Arr: %d, remain: %d,Total:%d, wait: %d.\n",
                clock, proc.id, proc.arrivaltime, proc.remainingtime, proc.runningtime, clock - proc.arrivaltime - proc.runningtime + proc.remainingtime);
    }
    else
    { // wait time = is basically dispatched-arrival;
        fprintf(filePointer, "At time %d, process %d started. Arr: %d, remain: %d,Total:%d, wait: %d.\n",
                clock, proc.id, proc.arrivaltime, proc.remainingtime, proc.runningtime, clock - proc.arrivaltime);
    }
    fclose(filePointer);
}

void LogFinishedRR(struct process proc, int noOfProcesses, int *runningTimeSum, float *WTASum, int *waitingTimeSum, float TAArray[], int *TAArrayIndex, int *shared)
{

    int clock = getClk();
    FILE *filePointer;
    float wta;
    filePointer = fopen("scheduler.log", "a");
    if (filePointer == NULL)
    {
        printf("Unable to open scheduler.log.\n");
        return;
    }
    if (proc.remainingtime == 0)
    {

        wta = ((float)clock - proc.arrivaltime) / (float)proc.runningtime;

        fprintf(filePointer, "At time %d, process %d finished. Arr: %d, remain: %d,Total:%d, wait: %d. TA %d WTA %.2f\n",
                clock, proc.id, proc.arrivaltime, proc.remainingtime, proc.runningtime, clock - proc.arrivaltime - proc.runningtime, clock - proc.arrivaltime, wta);
        *runningTimeSum += proc.runningtime;
        *WTASum += wta;
        *waitingTimeSum += clock - proc.arrivaltime - proc.runningtime;
        TAArray[*TAArrayIndex] = wta;
        *TAArrayIndex = *TAArrayIndex + 1;
        *shared = proc.id; // pointer to your “dead process” shared-memory slot, so external observers see which process just stopped or finished.
    }
    else
    {
        fprintf(filePointer, "At time %d, process %d stopped. Arr: %d, remain: %d,Total:%d, wait: %d.\n",
                clock, proc.id, proc.arrivaltime, proc.remainingtime, proc.runningtime, clock - proc.arrivaltime - proc.runningtime + proc.remainingtime);
    }
    fclose(filePointer);
}

void RoundRobin(int quantum, int processCount)
{
    // FILE *f = fopen("memory.log", "w");
    printf("Round Robin Scheduler\n");
    //--------------------------- this whole sections is for shared memory slots--------------------
    key_t runningProcKey = ftok("keys/Guirunningman", 'A');      // unique key for shared memory
    int runningID = shmget(runningProcKey, 4, IPC_CREAT | 0644); // 4 bytes for int
    // hena batlob  segment fl kernel bel key el e7na gbnah
    if ((long)runningID == -1)
    {
        perror("Error in creating shm!");
        exit(-1);
    }
    int *runningProcess = (int *)shmat(runningID, (void *)0, 0); // Attaches that segment into your address space, returning an int pointing at the shared 4 bytes.
    if ((long)runningProcess == -1)
    {
        perror("Error in attaching!"); // no proccess is running
        exit(-1);
    }

    *runningProcess = -1; // shared memory slot for the running process
    // 34an akhly el value b -1 mabd2yan  el howa mfysh had shaghal 3l cpu dlw2ty

    key_t deadProcKey = ftok("keys/Guideadman", 'A');
    int deadID = shmget(deadProcKey, 4, IPC_CREAT | 0644);
    if ((long)deadID == -1)
    {
        perror("Error in creating shm!");
        exit(-1);
    }
    int *deadProcess = (int *)shmat(deadID, (void *)0, 0);
    if ((long)deadProcess == -1)
    {
        perror("Error in attaching!");
        exit(-1);
    }
    *deadProcess = -1;
    // b2ol en l7d dlw2ty mfysh dead process 7asalet
    //----------------------------------------------------------------------------------------------
    clearLogFileRR();
    float TAArray[processCount]; // array to store the turnaround time of each process
    int TAArrayIndex = 0;
    int runningTimeSum = 0; // Sum of CPU busy‐time
    int iterator = 0;       // totalmemory = 1024;
    float WTASum = 0.0f;
    int waitingTimeSum = 0;
    int HasStartedArray[processCount + 1]; // Array to track if a process has started (flags)
    for (int i = 0; i <= processCount; i++)
    {
        HasStartedArray[i] = 0;
    }
    initSync();
    // Initialize Ready queue to receive processes from process generator

    int ReadyQueueID, SendQueueID, ReceiveQueueID;
    DefineKeys(&ReadyQueueID, &SendQueueID, &ReceiveQueueID);

    int quantumCounter = 0;
    int remainingProcesses = processCount;
    struct CircularList *Running_List = createCircularList();
   // struct process *Waiting = (struct process*)malloc(processCount * sizeof *Waiting);
   
    int clk = 0;
    // Main processing loop, keeps running until all processes are finished
    while (remainingProcesses > 0)
    {
        clk = getClk();
        printf("Current Clk: %d\n", clk);
        while (getSync() == 0)
        {
        } //
        setSync(0);

        // 1) Pull in any new arrivals
        bool StillArriving = true;
        while (StillArriving)
        {
            struct process rec;
            printf("Checking for new processes\n");
            struct timespec req = {0, 1000000}; // 1 ms
            nanosleep(&req, NULL);

            int received = msgrcv(ReadyQueueID, &rec, sizeof(rec), 0, IPC_NOWAIT);
            if (received != -1)
            {
                // Fork & enqueue
                pid_t pid = fork();
                if (pid == -1)
                {
                    perror("fork");
                }
                else if (pid == 0)
                {
                    char arg[20];
                    sprintf(arg, "%d", rec.runningtime);
                    char *args[] = {"./process.out", arg, NULL};
                    execv(args[0], args);
                    perror("execv");
                    exit(EXIT_FAILURE);
                }
                rec.pid = pid;
                insertAtEnd(Running_List, rec);
                displayList(Running_List);
            }
            else
            {
                StillArriving = false;
            }
        }

        // 2) If someone is running, get its updated remaining time
        if (!isEmpty(Running_List))
        {
            struct msgbuff receivedmsg;
            int got = msgrcv(ReceiveQueueID, &receivedmsg, sizeof(receivedmsg.msg), 0, IPC_NOWAIT);
            if (got != -1)
            {
                struct process p;
                getCurrent(Running_List, &p);
                p.remainingtime = receivedmsg.msg.remainingtime;
                changeCurrentData(Running_List, p);

                if (p.remainingtime == 0)
                {
                    printf("Process with ID: %d has finished\n", p.id);
                    LogFinishedRR(p, processCount,
                                  &runningTimeSum, &WTASum, &waitingTimeSum,
                                  TAArray, &TAArrayIndex, deadProcess);
                    struct process Terminated;
                    removeCurrent(Running_List, &Terminated);
                    displayList(Running_List);
                    remainingProcesses--;
                    quantumCounter = 0;
                    wait(NULL);
                }
            }

            // 3) Preempt when quantum expires
            if (quantumCounter == quantum)
            {
                // 1) Reset the counter so the next time slice starts fresh
                quantumCounter = 0;
            
                // 2) Mark the old process as no longer running
                struct process temp = Running_List->current->data;
                Running_List->current->data.flag = 0;
            
                // 3) Rotate the circular list to pick the next ready process
                changeCurrent(Running_List);
            
                // 4) Compare IDs: if we actually switched to a different process...
                struct process newCurrent = Running_List->current->data;
                if (temp.id != newCurrent.id)
                {
                    // 5a) Log that the old one was preempted (stopped)
                    LogFinishedRR(temp, processCount,
                                  &runningTimeSum, &WTASum, &waitingTimeSum,
                                  TAArray, &TAArrayIndex, deadProcess);
                }
                else
                {
                    // 5b) If the list only had one process, we're still running it
                    //     so just mark it as still running
                    Running_List->current->data.flag = 1;
                }
            }

            // 4) Dispatch the next process
            if (isEmpty(Running_List))
            {
                continue; // nothing to dispatch
            }
            struct msgbuff sendmsg = {
                .mtype = Running_List->current->data.pid,
                .msg = 1};
                //f you remove that sendmsg step, every child will just block forever on its msgrcv and your whole RR loop hangs. 

            if (msgsnd(SendQueueID, &sendmsg, sizeof(sendmsg.msg), IPC_NOWAIT) == -1)
            {
                perror("Error in send message");
                exit(-1);
            }

            struct process currentProcess;
            getCurrent(Running_List, &currentProcess);
            if (currentProcess.flag == 0) //his is the first tick we’re giving to that process right now.
            {
                currentProcess.flag = 1; //so subsequent ticks in the same time slice won’t re-log it.
                changeCurrentData(Running_List, currentProcess);
                LogStartedRR(currentProcess, runningProcess);
            }
            if (HasStartedArray[currentProcess.id] == 0)
            {
                HasStartedArray[currentProcess.id] = 1;
                currentProcess.starttime = clk;
                changeCurrentData(Running_List, currentProcess);
            }
            quantumCounter++;
        }

        // 5) Wait for the next clock tick
        while (getClk() == clk)
        {
        }
    }

    // Write out performance metrics…
    FILE *perf = fopen("scheduler.perf", "w");
    if (!perf)
    {
        perror("fopen scheduler.perf");
        exit(EXIT_FAILURE);
    }
    // … fprintfs …
    fclose(perf);

    // Cleanup
    shmdt(runningProcess);
    shmdt(deadProcess);
    destroyList(Running_List);
   // free(Waiting);
}
#endif