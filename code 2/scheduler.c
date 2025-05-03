// scheduler.c
#include "headers.h"
#include "MinHeap.h"
#include "RoundRobin.h"
#include "HPF.h"
#include "SRTN.h"


int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <proc_count> <algo> <quantum>\n", argv[0]);
        return 1;
    }

    initClk();
    int start = getClk();

    DefineKeys(&ReadyQueueID, &SendQueueID, &ReceiveQueueID);

    // Prepare log
    FILE *log = fopen("scheduler.log", "w");
    if (!log) { perror("fopen scheduler.log"); return 1; }
    fprintf(log, "# Scheduler start at time %d\n", start);
    fprintf(log, "# algo=%s, quantum=%s\n", argv[2], argv[3]);
    fclose(log);

    int n    = atoi(argv[1]);
    int algo = atoi(argv[2]);
    int q    = atoi(argv[3]);

    switch (algo) {
      case 1:
        runHPF(n);
        break;
    case 2:
    printf("Scheduler: Running HPF algorithm\n");
    runSRTN(NumberOfP);
    break;
    case 3:
        printf("Scheduler: Running RR algorithm\n");
        RoundRobin(q, n);

        break;
      default:
        fprintf(stderr, "Invalid algorithm %d\n", algo);
    }

    destroyClk(true);
    return 0;
}
