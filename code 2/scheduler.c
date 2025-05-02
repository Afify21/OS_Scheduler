#include "headers.h"
#include "MinHeap.h"
#include "HPF.h"

int main(int argc, char * argv[])
{
    // First ensure we have the correct arguments
    if (argc != 4) {
        printf("Error: Invalid number of arguments. Expected 3 arguments.\n");
        printf("Usage: ./scheduler.out <process_count> <algorithm> <quantum>\n");
        exit(-1);
    }
    
    // Initialize clock first - this must be done before anything else
    initClk();
    int startTime = getClk();
    printf("Scheduler: Clock initialized successfully at time %d\n", startTime);

    // Parse the command-line arguments
    int ProcessesCount = atoi(argv[1]);
    int AlgoNumber = atoi(argv[2]);
    int QuantumNumber = atoi(argv[3]);
    
    printf("Scheduler: Started with %d processes, algorithm %d, quantum %d\n", 
           ProcessesCount, AlgoNumber, QuantumNumber);

    // Initialize the message queues for communication
    DefineKeysProcess(&SendQueueID, &ReceiveQueueID);
    printf("Scheduler: Message queues initialized\n");
    
    // Create a log file for scheduler events
    FILE *logFile = fopen("scheduler.log", "w");
    if (!logFile) {
        perror("Error creating log file");
    } else {
        fprintf(logFile, "# Scheduler Log - Started at time %d\n", startTime);
        fprintf(logFile, "# Algorithm: %d, Quantum: %d\n", AlgoNumber, QuantumNumber);
        fclose(logFile);
    }
    
    // Choose and run the appropriate scheduling algorithm
    switch (AlgoNumber) {
        case 1:
            printf("Scheduler: Running HPF algorithm\n");
            runHPF(ProcessesCount);
            break;
        case 2:
            // Add SRTN implementation here when ready
            printf("Scheduler: SRTN algorithm not yet implemented\n");
            break;
        case 3:
            // Add RR implementation here when ready
            printf("Scheduler: RR algorithm not yet implemented\n");
            break;
        default:
            printf("Scheduler: Invalid algorithm choice %d\n", AlgoNumber);
            break;
    }
    
    // Clean up resources before exiting
    printf("Scheduler: Finished processing all %d processes\n", ProcessesCount);
    destroyClk(true);
    return 0;
}