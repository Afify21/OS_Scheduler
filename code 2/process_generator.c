#include "headers.h"

void clearResources(int);
void chooseAlgorithm(void);
void setUP_CLK_SCHDLR(void);
void sendInfo(void);

int algoChoice;
int quantum = -1; // default value

process processList[MAX_PROCESSES]; // Define processList here

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);

    // 1. Read the input files.
    FILE *F = fopen(argv[1], "r");
    if (!F)
    {
        perror("Can't Open File");
        return -1;
    }

    // Read process information from file
    NumberOfP = getNoOfProcessesFromInput(F);
    readProcessesFromFile(F, NumberOfP);
    fclose(F); // Close the file when done

    // 2. Ask the user for the chosen scheduling algorithm and its parameters
    chooseAlgorithm();

    // 3. Create the clock process and scheduler process
    // The clock process must be created first, then we initialize the clock
    // in the parent process before creating the scheduler
    setUP_CLK_SCHDLR();

    // 4. Send the process information to the scheduler at the appropriate time
    sendInfo();

    // 5. Clean up resources
    destroyClk(true);
    return 0;
}

void clearResources(int signum)
{
    // Clean up all message queues and shared memory
    msgctl(SendQueueID, IPC_RMID, NULL);
    destroyClk(true);
    exit(0);
}

void chooseAlgorithm(void)
{
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

    // Print what was selected
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

void setUP_CLK_SCHDLR(void)
{
    // First, make sure we don't have any existing message queues
    key_t send_key = ftok("keyfile", 65);
    key_t recv_key = ftok("keyfile", 66);
    int tmp_send = msgget(send_key, 0666);
    int tmp_recv = msgget(recv_key, 0666);

    if (tmp_send != -1)
        msgctl(tmp_send, IPC_RMID, NULL);
    if (tmp_recv != -1)
        msgctl(tmp_recv, IPC_RMID, NULL);

    // Setup message queues that will be used
    DefineKeys(&ReadyQueueID, &SendQueueID, &ReceiveQueueID);

    // 1. Create the clock process first
    int CLK_ID = fork();
    if (CLK_ID == -1)
    {
        perror("Error initializing clock");
        exit(-1);
    }
    else if (CLK_ID == 0)
    {
        // This is the child process for the clock
        execl("./clk.out", "clk.out", NULL);
        perror("Error executing clock");
        exit(-1);
    }

    // Give the clock process time to fully initialize the shared memory
    sleep(1);

    // 2. Initialize clock in the parent process
    initClk();
    printf("Clock initialized successfully at time %d\n", getClk());

    // 3. Create the scheduler process
    int SCHDLR_ID = fork();
    if (SCHDLR_ID == -1)
    {
        perror("Error initializing scheduler");
        exit(-1);
    }
    else if (SCHDLR_ID == 0)
    {
        // This is the child process for the scheduler
        char processesC[10];
        char algoNumber[5];
        char RRQ[5];

        sprintf(processesC, "%d", NumberOfP);
        sprintf(algoNumber, "%d", algoChoice);
        sprintf(RRQ, "%d", quantum);

        execl("./scheduler.out", "scheduler.out", processesC, algoNumber, RRQ, NULL);
        perror("Error executing scheduler");
        exit(-1);
    }

    // Wait a moment for the scheduler to initialize
    sleep(1);
}

void sendInfo(void) {
    int currentProcess = 0;
    struct msgbuff buf;

    // We've already created the message queues in setUP_CLK_SCHDLR

    printf("Starting to send process information to scheduler...\n");
    while (currentProcess < NumberOfP) {
        int currentTime = getClk();

        if (processList[currentProcess].arrivaltime <= currentTime)
        {
            buf.mtype = processList[currentProcess].id;
            buf.msg.id = processList[currentProcess].id;
            buf.msg.remainingtime= processList[currentProcess].remainingtime;
            buf.msg.priority = processList[currentProcess].priority;
            buf.msg.runningtime = processList[currentProcess].runningtime;

            if (msgsnd(SendQueueID, &buf, sizeof(buf.msg), 0) == -1)
            {
                perror("msgsnd failed");
            } else {
                printf("Sent process %d (remaining: %d, priority: %d, running: %d) at time %d\n",
                    processList[currentProcess].id,
                    processList[currentProcess].remainingtime,
                    processList[currentProcess].priority,
                    processList[currentProcess].runningtime,
                    currentTime);
            }

            currentProcess++;
        }
        else
        {
            // Sleep for a short time to avoid busy waiting
            usleep(100000); // 100ms sleep is more reasonable
        }
    }

    printf("All processes have been sent to the scheduler\n");
    while(1);
}