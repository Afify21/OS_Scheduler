#include "headers.h"


int main(int argc, char * argv[])
{
    initClk();

    int AlgoNumber;
    int QuantumNumber;
    int ProcessesCount;

    ProcessesCount=atoi(argv[1]);
    AlgoNumber=atoi(argv[2]);
    QuantumNumber=atoi(argv[3]);


    
    
    key_t key = ftok("scheduler.c", 123); // Use a unique key
    int SendQueueID = msgget(key, 0666 | IPC_CREAT);
    if (SendQueueID == -1) {
    perror("msgget failed");
    exit(1);
}
    //TODO implement the scheduler :)
    // 2. Main scheduling loop 
    while (1) {
        struct msgbuff buf;
        if (msgrcv(SendQueueID, &buf, sizeof(buf.msg), 0, IPC_NOWAIT) != -1) {
            printf("Scheduler: Received process %ld (remaining: %d)\n", buf.mtype, buf.msg);
            // TODO: Implement scheduling logic (HPF, SRTN, RR)
        }
        usleep(1000); // Avoid busy waiting
    }
    
    destroyClk(true);
}

