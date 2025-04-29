#include "headers.h"

void clearResources(int);
void chooseAlgorithm(void);
void createSchedulerAndClock(void);
void sendInfo(int);

int main(int argc, char *argv[])
{

    int NumberOfP;

    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.

    FILE *F = fopen(argv[1], "r");

if (!F)
{
    perror("Cant Open File");
    return -1;
}
else
{
    NumberOfP = getNoOfProcessesFromInput(F);
    readProcessesFromFile(F, NumberOfP); 
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


void createSchedulerandClock(void){

}
