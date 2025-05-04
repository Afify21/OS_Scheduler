#include "headers.h"
#include <stdio.h>
#include <stdlib.h>

/* Each process simulates a CPU-bound job: receive one "tick" from the scheduler,
   decrement its remaining time, report back, and wait for the next clock tick. */

int main(int argc, char *argv[])
{
    // Validate we got exactly one runtime argument from the scheduler
    if (argc != 2)
    {
        perror("Invalid number of arguments");
        return -1;
    }
    printf("[DEBUG] Process %d started with runtime %s\n", getpid(), argv[1]);
    
    // Attach to the simulated clock
    initClk();
    
    // Prepare our two message-queue handles
    int SendQueueID, ReceiveQueueID, ReadyQueueID;
    DefineKeys(&ReadyQueueID, &SendQueueID, &ReceiveQueueID);
    
    // Initialize our remaining-time counter
    int remainingTime = atoi(argv[1]);
    int currentClock = getClk();
    int lastClock = currentClock;
    int isRunning = 0; // Flag to track if process is currently running

    // Main loop run until we've used up all our ticks
    while (remainingTime > 0)
    {
        // Sync with current clock tick
        currentClock = getClk();

        // Use a single buffer for both receiving and sending
        struct msgbuff buf;

        // Check for messages from scheduler (non-blocking)
        int received = msgrcv(
            SendQueueID,
            &buf,
            sizeof(buf.msg),
            getpid(),
            IPC_NOWAIT); // Use non-blocking to avoid deadlock

        if (received != -1)
        {
            // We got the "go" signal - we are now running
            isRunning = 1;
            printf("[DEBUG] Process %d received run signal at clock time %d\n", getpid(), currentClock);
        }

        // If clock has advanced and we're running, decrement remaining time
        if (currentClock > lastClock && isRunning)
        {
            // Update the time at which we last processed a clock tick
            lastClock = currentClock;
            
            // Only decrement if we're running
            remainingTime--;
            printf("[DEBUG] Process %d: Clock tick %d, Remaining time = %d\n", 
                   getpid(), currentClock, remainingTime);
            
            // Send back the updated remaining time immediately after decrementing
            buf.mtype = getpid();
            buf.msg.remainingtime = remainingTime;
            buf.msg.pid = getpid();
            // Include the exact clock time when this update is happening
            buf.msg.lasttime = currentClock;

            // Try to send the message multiple times if needed
            int retryCount = 0;
            while (msgsnd(ReceiveQueueID, &buf, sizeof(buf.msg), 0) == -1 && retryCount < 3) {
                perror("Error sending message back to scheduler, retrying...");
                usleep(1000); // Small delay before retry
                retryCount++;
            }
            
            if (retryCount >= 3) {
                perror("Failed to send message after multiple retries");
                exit(-1);
            }
            
            printf("[DEBUG] Process %d sent update at clock %d: remaining time = %d\n", 
                   getpid(), currentClock, remainingTime);
            
            // If we have no more time, we're done
            if (remainingTime <= 0)
            {
                printf("[DEBUG] Process %d completed execution at clock time %d\n", 
                       getpid(), currentClock);
                break;
            }
        }

        // Small sleep to reduce CPU usage
        usleep(1000);
    }

    // Send final completion message
    struct msgbuff finalBuf;
    finalBuf.mtype = getpid();
    finalBuf.msg.remainingtime = 0;
    finalBuf.msg.pid = getpid();
    finalBuf.msg.lasttime = getClk();
    
    if (msgsnd(ReceiveQueueID, &finalBuf, sizeof(finalBuf.msg), 0) == -1) {
        perror("Error sending final completion message");
    }

    // We're done—detach from the clock service and exit
    printf("[DEBUG] Process %d finished and detaching from clock\n", getpid());
    destroyClk(false);
    return 0;
}