#include "headers.h"

void clearResources(int);
void chooseAlgorithm(void);
void createSchedulerAndClock(void);
void sendInfo(void);

int main(int argc, char *argv[])
{

    int NumberOfP;

    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.

    FILE *F=fopen(argv[1],"r");

    if(!F)
    {
        perror("Cant Open File");
        return -1;
    }
    else
    {
        NumberOfP=getNoOfProcessesFromInput(F);
    }






    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
   chooseAlgorithm();
    // 3. Initiate and create the scheduler and clock processes.
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    int x = getClk();
    printf("current time is %d\n", x);
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // test // 6. Send the information to the scheduler at the appropriate time.
    // nice // 7. Clear clock resources
    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources
    destroyClk(true);
}

void clearResources(int signum)
{
    // TODO Clears all resources in case of interruption
}

void chooseAlgorithm(void)
{
    int algoChoice;
    int quantum = -1; // default value

    printf("Choose a scheduling algorithm:\n");
    printf("1. HPF (Highest Priority First)\n");
    printf("2. SRTN (Shortest Remaining Time)\n");
    printf("3. RR  (Round Robin)\n");
    printf("Enter your choice (1-3): ");
    scanf("%d", &algoChoice);

    if (algoChoice == 3) // RR needs quantum
    {
        printf("Enter the time quantum: ");
        scanf("%d", &quantum);
    }

    // Example to print what was selected
    if (algoChoice == 1)
        printf("You selected HPF.\n");
    else if (algoChoice == 2)
        printf("You selected SRT.\n");
    else if (algoChoice == 3)
        printf("You selected RR with quantum = %d.\n", quantum);
    else
    {
        printf("Invalid choice. Exiting.\n");
        exit(1);
    }
}
void createSchedulerandClock(void){

}
void sendInfo(void){
    
}