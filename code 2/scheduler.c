// scheduler.c
#include "headers.h"
#include "MinHeap.h"
#include "RoundRobin.h"
#include "HPF.h"
#include "SRTN.h"
#include "Defined_DS.h"

// Global buddy allocator
BuddyAllocator *memoryAllocator;

int main(int argc, char *argv[])
{
  if (argc != 4)
  {
    fprintf(stderr, "Usage: %s <proc_count> <algo> <quantum>\n", argv[0]);
    return 1;
  }

  initClk();
  int start = getClk();

  DefineKeys(&ReadyQueueID, &SendQueueID, &ReceiveQueueID);

  // Initialize memory allocator with 1024 bytes
  memoryAllocator = initBuddyAllocator(1024);

  // Prepare log
  FILE *log = fopen("scheduler.log", "w");
  if (!log)
  {
    perror("fopen scheduler.log");
    return 1;
  }
  fprintf(log, "# Scheduler start at time %d\n", start);
  fprintf(log, "# algo=%s, quantum=%s\n", argv[2], argv[3]);
  fclose(log);

  int n = atoi(argv[1]);
  int algo = atoi(argv[2]);
  int q = atoi(argv[3]);

  switch (algo)
  {
  case 1:
    runHPF(n);
    break;
  case 2:
    printf("Scheduler: Running HPF algorithm\n");
    runSRTN(n);
    break;
  case 3:
    printf("Scheduler: Running RR algorithm\n");
    RoundRobin(q, n);

    break;
  default:
    fprintf(stderr, "Invalid algorithm %d\n", algo);
  }

  // Close memory log file
  if (memoryAllocator && memoryAllocator->memory_log)
  {
    fclose(memoryAllocator->memory_log);
  }

  // Free memory allocator
  if (memoryAllocator)
  {
    free(memoryAllocator);
  }

  destroyClk(true);
  return 0;
}
