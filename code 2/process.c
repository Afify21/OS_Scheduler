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
    DefineKeysProcess(&SendQueueID, &ReceiveQueueID); // calls message get twice
    // SendQueueID:   scheduler → process “here’s your turn”
    // ReceiveQueueID: process → scheduler “here’s my remaining time”
    // Initialize our remaining-time counter
    int remainingTime = atoi(argv[1]);
    int currentClock = getClk();
    // Main loop run until we’ve used up all our ticks
    while (remainingTime > 0)
    { //  Sync with current clock tick
        currentClock = getClk();
        // Wait (block) until scheduler sends a message with mtype == our PID
        struct msgbuff turn;
        // zero el fl msg recieve bt block (wait )l7d ma a recieve sth from the scheduler
        int received = msgrcv(SendQueueID, &turn, sizeof(turn.msg), getpid(), 0); // returns the number of bytes
        if (received != -1)                                                       // indicates an error(so if not error minus ya basha)
        {
            // We got the “go” for one tick
            remainingTime--;
        }
        // Report new remaining time back to scheduler
        struct msgbuff rem;
        rem.mtype = getpid();    // so scheduler knows whose update this is
        rem.msg = remainingTime; // how many ticks left
        msgsnd(ReceiveQueueID, &rem, sizeof(rem.msg), 0);
        // Busy-wait until the clock advances before looping again
        while (currentClock == getClk())
        {
            /* just makes sure en there is no incrementation aw decrementation happens kaza mra per
            1 tick */
        }
    }
    // We’re done—detach from the clock service and exit
    destroyClk(false);
    return 0;
}
