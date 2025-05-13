#ifndef MINHEAP_H
#define MINHEAP_H

#include "headers.h" 

// Assume "process" is defined in "headers.h" with at least:
// typedef struct { int pid; int remainingtime; int priority; } process;

typedef struct MinHeap {
    process *arr;
    int capacity;
    int currentSize;
} MinHeap;

// --- Core Functions ---
MinHeap *createMinHeap(int capacity);
void destroyMinHeap(MinHeap *minHeap);
//int isEmpty(MinHeap *minHeap);

// --- Heap Operations ---
void insertMinHeap_SRTN(MinHeap *minHeap, process p);  // For SRTN (Shortest Remaining Time)
void insertMinHeap_HPF(MinHeap *minHeap, process p);   // For HPF (Highest Priority First)
process deleteMinSRTN(MinHeap *minHeap);               // Extract min for SRTN
process deleteMinHPF(MinHeap *minHeap);                // Extract min for HPF
process getMin(MinHeap *minHeap);                      // Peek min without removal

// --- Helper Functions ---
static void MinHeapifySRTN(MinHeap *minHeap, int index);
static void MinHeapifyHPF(MinHeap *minHeap, int index);
static inline int Parent(int index);
static inline int LeftChild(int index);
static inline int RightChild(int index);

// ================= Implementation =================
MinHeap *createMinHeap(int capacity) {
    MinHeap *minHeap = (MinHeap *)malloc(sizeof(MinHeap));
    if (!minHeap) return NULL;

    minHeap->arr = (process *)malloc(capacity * sizeof(process));
    if (!minHeap->arr) {
        free(minHeap);
        return NULL;
    }

    minHeap->capacity = capacity;
    minHeap->currentSize = 0;
    return minHeap;
}

void destroyMinHeap(MinHeap *minHeap) {
    if (minHeap) {
        free(minHeap->arr);
        free(minHeap);
    }
}

int HeapisEmpty(MinHeap *minHeap) {
    return minHeap->currentSize == 0;
}

// --- Heapify Functions ---
void MinHeapifySRTN(MinHeap *minHeap, int index) {
    int smallest = index;
    int left = LeftChild(index);
    int right = RightChild(index);

    if (left < minHeap->currentSize && 
        minHeap->arr[left].remainingtime < minHeap->arr[smallest].remainingtime) {
        smallest = left;
    }
    if (right < minHeap->currentSize && 
        minHeap->arr[right].remainingtime < minHeap->arr[smallest].remainingtime) {
        smallest = right;
    }
    if (smallest != index) {
        process temp = minHeap->arr[index];
        minHeap->arr[index] = minHeap->arr[smallest];
        minHeap->arr[smallest] = temp;
        MinHeapifySRTN(minHeap, smallest);
    }
}

void MinHeapifyHPF(MinHeap *minHeap, int index) {
    int smallest = index;
    int left = LeftChild(index);
    int right = RightChild(index);

    if (left < minHeap->currentSize && 
        minHeap->arr[left].priority < minHeap->arr[smallest].priority) {
        smallest = left;
    }
    if (right < minHeap->currentSize && 
        minHeap->arr[right].priority < minHeap->arr[smallest].priority) {
        smallest = right;
    }
    if (smallest != index) {
        process temp = minHeap->arr[index];
        minHeap->arr[index] = minHeap->arr[smallest];
        minHeap->arr[smallest] = temp;
        MinHeapifyHPF(minHeap, smallest);
    }
}

// --- Insertion ---
void insertMinHeap_SRTN(MinHeap *minHeap, process p) {
    if (minHeap->currentSize == minHeap->capacity) {
        printf("Heap is full, cannot insert process\n");
        return;
    }
    minHeap->currentSize++;
    int index = minHeap->currentSize - 1;
    minHeap->arr[index] = p;

    while (index != 0 && 
           minHeap->arr[Parent(index)].remainingtime > minHeap->arr[index].remainingtime) {
        process temp = minHeap->arr[index];
        minHeap->arr[index] = minHeap->arr[Parent(index)];
        minHeap->arr[Parent(index)] = temp;
        index = Parent(index);
    }
}

void insertMinHeap_HPF(MinHeap *minHeap, process p) {
    if (minHeap->currentSize == minHeap->capacity) {
        printf("Heap is full, cannot insert process\n");
        return;
    }
    minHeap->currentSize++;
    int index = minHeap->currentSize - 1;
    minHeap->arr[index] = p;

    while (index != 0 && 
           minHeap->arr[Parent(index)].priority > minHeap->arr[index].priority) {
        process temp = minHeap->arr[index];
        minHeap->arr[index] = minHeap->arr[Parent(index)];
        minHeap->arr[Parent(index)] = temp;
        index = Parent(index);
    }
}

// --- Extraction ---
process deleteMinSRTN(MinHeap *minHeap) {
    if (minHeap->currentSize == 0) {
        printf("Heap is empty\n");
        process empty = {0};
        return empty;
    }
    process root = minHeap->arr[0];
    minHeap->arr[0] = minHeap->arr[minHeap->currentSize - 1];
    minHeap->currentSize--;
    MinHeapifySRTN(minHeap, 0);
    return root;
}

process deleteMinHPF(MinHeap *minHeap) {
    if (minHeap->currentSize == 0) {
        printf("Heap is empty\n");
        process empty = {0};
        return empty;
    }
    process root = minHeap->arr[0];
    minHeap->arr[0] = minHeap->arr[minHeap->currentSize - 1];
    minHeap->currentSize--;
    MinHeapifyHPF(minHeap, 0);
    return root;
}

process getMin(MinHeap *minHeap) {
    if (minHeap->currentSize == 0) {
        printf("Heap is empty\n");
        process empty = {0};
        return empty;
    }
    return minHeap->arr[0];
}

// --- Helper Functions ---
int Parent(int index) { return (index - 1) / 2; }
int LeftChild(int index) { return 2 * index + 1; }
int RightChild(int index) { return 2 * index + 2; }

#endif 