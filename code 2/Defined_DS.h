#ifndef DEFINED_DS_H
#define DEFINED_DS_H

#include "headers.h"

// ========================= MEMORY ========================
typedef struct MemoryBlock
{
    int start_address;
    int size;
    bool is_free;
    int process_id; // ID of the process using this block
    struct MemoryBlock *next;
} MemoryBlock;

typedef struct BuddyAllocator
{
    MemoryBlock *memory_list; // linked list of the memory block
    int total_size;           // Total memory available.
    FILE *memory_log;         // File to log memory operations.
} BuddyAllocator;

// Initialize buddy allocator
BuddyAllocator *initBuddyAllocator(int total_size)
{
    BuddyAllocator *allocator = (BuddyAllocator *)malloc(sizeof(BuddyAllocator));
    allocator->total_size = total_size;

    // Create initial free block
    allocator->memory_list = (MemoryBlock *)malloc(sizeof(MemoryBlock));
    allocator->memory_list->start_address = 0;
    allocator->memory_list->size = total_size;
    allocator->memory_list->is_free = true;
    allocator->memory_list->process_id = -1; // No process assigned
    allocator->memory_list->next = NULL;

    // Open memory log file
    allocator->memory_log = fopen("memory.log", "w");
    fprintf(allocator->memory_log, "#At time x allocated y bytes for process z from i to j\n");
    return allocator;
}

// Find smallest power of 2 that fits the requested size
int getNextPowerOf2(int size)
{
    // For simplicity, we'll just use fixed sizes
    // For sizes between 1-128, allocate 128 bytes
    // For sizes between 129-256, allocate 256 bytes
    if (size <= 64)
        return 64;
    if (size <= 128)
        return 128;
    else
        return 256;
}

// Debug function to print the current state of memory
void printMemoryState(BuddyAllocator *allocator)
{
    printf("===== Memory State =====\n");
    MemoryBlock *current = allocator->memory_list;
    int i = 0;

    while (current != NULL)
    {
        printf("Block %d: addr=%d, size=%d, free=%d, pid=%d\n",
               i++, current->start_address, current->size,
               current->is_free, current->process_id);
        current = current->next;
    }
    printf("=======================\n");
}

// Checks if the process already owns a block — frees it if so.
// Searches for a suitable free block:
// Exact match? → Allocate it.
// Larger block? → Recursively split it in half until the size matches
//  Allocate memory
MemoryBlock *allocateMemory(BuddyAllocator *allocator, int size, int process_id)
{
    // Check if this process already has memory allocated
    MemoryBlock *current = allocator->memory_list;
    while (current != NULL)
    {
        if (!current->is_free && current->process_id == process_id)
        {
            printf("WARNING: Process %d already has memory allocated at address %d, size %d\n",
                   process_id, current->start_address, current->size);

            // Free the existing block to avoid memory leaks
            printf("Freeing existing block for process %d to avoid memory leaks\n", process_id);
            current->is_free = true;
            current->process_id = -1;

            // Don't return the block, continue with allocation
            break;
        }
        current = current->next;
    }

    // Simplified allocation strategy for debugging
    int required_size = getNextPowerOf2(size);
    printf("Required size for process %d: %d bytes\n", process_id, required_size);

    // Print the current memory state
    printf("Memory state before allocation attempt:\n");
    printMemoryState(allocator);

    // First, try to find an exact-sized free block
    current = allocator->memory_list;
    MemoryBlock *best_fit = NULL;

    // First pass: look for an exact match
    while (current != NULL)
    {
        if (current->is_free && current->size == required_size)
        {
            // Found an exact match
            printf("Found exact match block at address %d, size %d\n",
                   current->start_address, current->size);

            current->is_free = false;
            current->process_id = process_id;

            fprintf(allocator->memory_log, "At time %d allocated %d bytes for process %d from %d to %d\n",
                    getClk(), size, process_id, current->start_address,
                    current->start_address + current->size - 1);
            fflush(allocator->memory_log);

            return current;
        }

        // Keep track of the smallest free block that's big enough
        if (current->is_free && current->size > required_size)
        {
            if (best_fit == NULL || current->size < best_fit->size)
            {
                best_fit = current;
            }
        }

        current = current->next;
    }

    // If no exact match, use the best fit and split it
    if (best_fit != NULL)
    {
        printf("Found best fit block at address %d, size %d\n",
               best_fit->start_address, best_fit->size);

        // Split the block until it's the right size
        while (best_fit->size > required_size)
        {
            int new_size = best_fit->size / 2;
            MemoryBlock *buddy = (MemoryBlock *)malloc(sizeof(MemoryBlock));

            buddy->size = new_size;
            buddy->start_address = best_fit->start_address + new_size;
            buddy->is_free = true;
            buddy->process_id = -1;
            buddy->next = best_fit->next;

            best_fit->size = new_size;
            best_fit->next = buddy;

            printf("Split block: now have blocks of size %d at addresses %d and %d\n",
                   new_size, best_fit->start_address, buddy->start_address);
        }

        // Mark the block as allocated
        best_fit->is_free = false;
        best_fit->process_id = process_id;

        fprintf(allocator->memory_log, "At time %d allocated %d bytes for process %d from %d to %d\n",
                getClk(), size, process_id, best_fit->start_address,
                best_fit->start_address + best_fit->size - 1);
        fflush(allocator->memory_log);

        printf("Successfully allocated block at address %d, size %d for process %d\n",
               best_fit->start_address, best_fit->size, process_id);

        return best_fit;
    }

    printf("No suitable memory block found for process %d\n", process_id);
    return NULL;
}

bool areBuddies(MemoryBlock *block1, MemoryBlock *block2)
{
    if (!block1 || !block2)
        return false;
    if (block1->size != block2->size)
        return false; // buddies must be the same size

    // Check if blocks are properly aligned buddies so they can merge
    return (block1->start_address ^ block2->start_address) == block1->size; // XOR: 0 ^ 128 = 128 → they are buddies
}

// Free memory
// Marks the block as free and logs it.
// Looks for a "buddy" block (same size, adjacent, and free).//
// If buddies are found → merge them and repeat.
// Verifies that the block has been properly freed.
void freeMemory(BuddyAllocator *allocator, MemoryBlock *block, int process_id)
{
    if (!block)
    {
        printf("Error: Attempted to free NULL block for process %d\n", process_id);
        return;
    }

    printf("Freeing block at address %d, size %d for process %d\n",
           block->start_address, block->size, process_id);

    // Mark the block as free
    block->is_free = true;
    block->process_id = -1; // Reset process ID

    fprintf(allocator->memory_log, "At time %d freed %d bytes from process %d from %d to %d\n",
            getClk(), block->size, process_id, block->start_address,
            block->start_address + block->size - 1);
    fflush(allocator->memory_log);

    // Keep merging buddies until no more merges are possible
    bool merged;
    do
    {
        merged = false;
        MemoryBlock *current = allocator->memory_list;

        while (current && current->next)
        {
            MemoryBlock *buddy = current->next;

            // Check if these are buddies that can be merged
            if (current->is_free && buddy->is_free && areBuddies(current, buddy))
            {
                printf("Merging buddies: block at %d (size %d) with block at %d (size %d)\n",
                       current->start_address, current->size,
                       buddy->start_address, buddy->size);

                // Merge the buddies
                current->size *= 2;
                current->next = buddy->next;
                free(buddy);
                merged = true;
                break;
            }

            current = current->next;
        }
    } while (merged);

    printf("Memory after freeing and merging:\n");
    printMemoryState(allocator);

    // Verify that the block was actually freed
    MemoryBlock *verify_current = allocator->memory_list;
    bool found_freed_block = false;

    while (verify_current)
    {
        if (verify_current->start_address == block->start_address)
        {
            if (verify_current->is_free)
            {
                found_freed_block = true;
                printf("Verified: Block at address %d is now free\n", verify_current->start_address);
            }
            else
            {
                printf("ERROR: Block at address %d is still marked as allocated!\n", verify_current->start_address);
            }
            break;
        }
        verify_current = verify_current->next;
    }

    if (!found_freed_block)
    {
        printf("ERROR: Could not find the freed block at address %d in the memory list!\n", block->start_address);
    }
}

// PCB structure to track process information including memory
typedef struct PCB
{
    process process_data; // Process details
    int state;            // Process state
    int remaining_time;   // Remaining runtime
    int start_time;       // Start time
    int finish_time;      // Finish time
    int waiting_time;     // Waiting time
    int TA;               // Turnaround time
    float WTA;            // Weighted turnaround time
    int Last_execution;   // Last execution time
    int executionTime;    // Total execution time
    pid_t pid;            // Process ID after fork

    // Memory management
    MemoryBlock *memory_block; // Pointer to allocated memory block
} PCB;

// Waiting Queue for processes
typedef struct NodeMem
{
    PCB *pcb;
    struct NodeMem *next;
} NodeMem;

typedef struct WaitingQueue
{
    NodeMem *front;
    NodeMem *rear;
    int size;
} WaitingQueue;

WaitingQueue *createWaitingQueue()
{
    WaitingQueue *waitingQ = (WaitingQueue *)malloc(sizeof(WaitingQueue));
    waitingQ->front = waitingQ->rear = NULL;
    waitingQ->size = 0;
    return waitingQ;
}

void enqueueWaiting(WaitingQueue *waitingQ, PCB *pcb)
{
    NodeMem *newNode = (NodeMem *)malloc(sizeof(NodeMem));
    newNode->pcb = pcb;
    newNode->next = NULL;

    if (waitingQ->rear == NULL)
    {
        waitingQ->front = waitingQ->rear = newNode;
    }
    else
    {
        waitingQ->rear->next = newNode;
        waitingQ->rear = newNode;
    }

    waitingQ->size++;
}

PCB *dequeueWaiting(WaitingQueue *waitingQ)
{
    if (waitingQ->front == NULL)
    {
        return NULL;
    }
    else
    {
        NodeMem *temp = waitingQ->front;
        PCB *dequeued = temp->pcb;
        waitingQ->front = waitingQ->front->next;

        if (waitingQ->front == NULL)
        {
            waitingQ->rear = NULL;
        }

        free(temp);
        waitingQ->size--;
        return dequeued;
    }
}

#endif // DEFINED_DS_H
