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
    DefineKeys(&ReadyQueueID, &SendQueueID, &ReceiveQueueID); // calls msgget() twice
    // SendQueueID:   scheduler → process "here's your turn"
    // ReceiveQueueID: process → scheduler "here's my remaining time"
    // ready queue hena mlosh lazma

    // Initialize our remaining-time counter
    int remainingTime = atoi(argv[1]);
    int currentClock = getClk();

    // Main loop run until we've used up all our ticks
    while (remainingTime > 0)
    {
        // Sync with current clock tick
        currentClock = getClk();

        // Use a single buffer for both receiving and sending
        struct msgbuff buf;

        printf("[DEBUG] Process %d waiting for msgrcv on SendQueueID\n", getpid());
        // Wait for scheduler's message
        int received = msgrcv(
            SendQueueID,
            &buf,
            sizeof(buf.msg),
            getpid(),
            0);
        printf("[DEBUG] Process %d msgrcv returned: %d\n", getpid(), received);

        if (received != -1)
        {
            // We got the "go" for one tick
            remainingTime--;
            printf("[DEBUG] Process %d: Remaining time = %d\n", getpid(), remainingTime);
        }

        // Send back the updated remaining time
        buf.mtype = getpid();
        buf.msg.remainingtime = remainingTime;
        buf.msg.id = getpid();

        printf("[DEBUG] Process %d sending msgsnd to ReceiveQueueID\n", getpid());
        if (msgsnd(ReceiveQueueID, &buf, sizeof(buf.msg), 0) == -1)
        {
            perror("Error sending message back to scheduler");
            exit(-1);
        }
        printf("[DEBUG] Process %d sent message to scheduler\n", getpid());

        // Busy-wait until the clock advances before looping again
        while (currentClock == getClk())
        {
            usleep(1000); // Small sleep to reduce CPU usage
        }
    }

    // We're done—detach from the clock service and exit
    printf("[DEBUG] Process %d finished and detaching from clock\n", getpid());
    destroyClk(false);
    return 0;
} // trem
