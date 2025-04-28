#include "headers.h"
#include <stdio.h>
#include <stdlib.h>

/* Each process simulates a CPU-bound job: receive one “tick” from the scheduler,
   decrement its remaining time, report back, and wait for the next clock tick. */

int main(int argc, char *argv[])
{
    // 1. Validate we got exactly one runtime argument from the scheduler
    if (argc != 2)
    {
        perror("Invalid number of arguments");
        return -1;
    }
    // Attach to the simulated clock
    initClk();

    // Prepare our two message-queue handles
    int SendQueueID, ReceiveQueueID;
    DefineKeysProcess(&SendQueueID, &ReceiveQueueID); // calls msgget() twice
    // SendQueueID:   scheduler → process “here’s your turn”
    // ReceiveQueueID: process → scheduler “here’s my remaining time”

    // Initialize our remaining-time counter
    int remainingTime = atoi(argv[1]);
    int currentClock = getClk();

    // Main loop run until we’ve used up all our ticks
    while (remainingTime > 0)
    {
        // Sync with current clock tick
        currentClock = getClk();

        // Use a single buffer for both receiving and sending
        struct msgbuff buf;

        // Wait (block) until scheduler sends a message with mtype == our PID
        // This returns the number of bytes received (or -1 on error)
        int received = msgrcv(
            SendQueueID,     // which queue to read from
            &buf,            // our single buffer
            sizeof(buf.msg), // only the payload size
            getpid(),        // filter: mtype must match our PID
            0                // blocking receive
        );
        if (received != -1) // if no error
        {
            // We got the “go” for one tick
            remainingTime--;
        }

        // Prepare the same buffer to report back
        buf.mtype = getpid();    // so scheduler knows whose update this is
        buf.msg = remainingTime; // how many ticks left

        // Send the updated remaining time back to scheduler
        msgsnd(
            ReceiveQueueID,  // which queue to write to
            &buf,            // reusing the buffer
            sizeof(buf.msg), // only the payload size
            0                // blocking send on error (rare)
        );

        // Busy-wait until the clock advances before looping again
        while (currentClock == getClk())
        {
            /* Just ensures no extra loops per tick */
        }
    }

    // We’re done—detach from the clock service and exit
    destroyClk(false);
    return 0;
}
