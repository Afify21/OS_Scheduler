#ifndef MINHEAP_H
#define MINHEAP_H

#include "headers.h"

typedef struct MinHeap {
    process *arr;
    int capacity;
    int currentSize;
}MinHeap;

MinHeap* createMinHeap(int capacity);

void MinHeapifySRTN(MinHeap* minHeap,int index);
void MinHeapifyHPF(MinHeap* minHeap,int index);   

int Parent(int index);
int LeftChild(int index);
int RightChild(int index); 

void insertMinHeap_SRTN(MinHeap* minHeap, process p);
void insertMinHeap_HPF(MinHeap* minHeap, process p);

process getMin(MinHeap* minHeap);
process* getMinPtr(MinHeap* minHeap);




MinHeap* createMinHeap(int capacity) {
    MinHeap* minHeap = (MinHeap*)malloc(sizeof(MinHeap));
    minHeap->capacity = capacity;
    minHeap->currentSize = 0;
    minHeap->arr = (process*)malloc(capacity * sizeof(process));
    return minHeap;
}

int Parent(int index) {
    return (index - 1) / 2;
}
int LeftChild(int index) {
    return 2 * index + 1;
}
int RightChild(int index) {
    return 2 * index + 2;
}

void MinHeapifySRTN(MinHeap* minHeap,int index) {
    int smallest = index;
    int left = LeftChild(index);
    int right = RightChild(index);

    if (left < minHeap->currentSize && minHeap->arr[left].remainingTime < minHeap->arr[smallest].remainingTime) {
        smallest = left;
    }
    if (right < minHeap->currentSize && minHeap->arr[right].remainingTime < minHeap->arr[smallest].remainingTime) {
        smallest = right;
    }
    if (smallest != index) {
        process temp = minHeap->arr[index];
        minHeap->arr[index] = minHeap->arr[smallest];
        minHeap->arr[smallest] = temp;
        MinHeapifySRTN(minHeap, smallest);
    }
}
void MinHeapifyHPF(MinHeap* minHeap,int index) {
    int smallest = index;
    int left = LeftChild(index);
    int right = RightChild(index);

    if (left < minHeap->currentSize && minHeap->arr[left].priority < minHeap->arr[smallest].priority) {
        smallest = left;
    }
    if (right < minHeap->currentSize && minHeap->arr[right].priority < minHeap->arr[smallest].priority) {
        smallest = right;
    }
    if (smallest != index) {
        process temp = minHeap->arr[index];
        minHeap->arr[index] = minHeap->arr[smallest];
        minHeap->arr[smallest] = temp;
        MinHeapifyHPF(minHeap, smallest);
    }
}

void insertMinHeap_SRTN(MinHeap* minHeap, process p) {
    if (minHeap->currentSize == minHeap->capacity) {
        printf("Heap is full, cannot insert process\n");
        return;
    }
    minHeap->currentSize++;
    int index = minHeap->currentSize - 1;
    minHeap->arr[index] = p;

    while (index != 0 && minHeap->arr[Parent(index)].remainingTime > minHeap->arr[index].remainingTime) {
        process temp = minHeap->arr[index];
        minHeap->arr[index] = minHeap->arr[Parent(index)];
        minHeap->arr[Parent(index)] = temp;
        index = Parent(index);
    }   
}

void insertMinHeap_HPF(MinHeap* minHeap, process p) {
    if (minHeap->currentSize == minHeap->capacity) {
        printf("Heap is full, cannot insert process\n");
        return;
    }
    minHeap->currentSize++;
    int index = minHeap->currentSize - 1;
    minHeap->arr[index] = p;

    while (index != 0 && minHeap->arr[Parent(index)].priority > minHeap->arr[index].priority) {
        process temp = minHeap->arr[index];
        minHeap->arr[index] = minHeap->arr[Parent(index)];
        minHeap->arr[Parent(index)] = temp;
        index = Parent(index);
    }   
}

process getMin(MinHeap* minHeap); {
    if (minHeap->currentSize == 0) {
        printf("Heap is empty\n");
        return NULL;
    }
    return minHeap->arr[0];
}

process* getMinPtr(MinHeap* minHeap) {
    if (minHeap->currentSize == 0) {
        printf("Heap is empty\n");
        return NULL;
    }
    return &minHeap->arr[0];
}

void deleteMinSRTN(MinHeap* minHeap, process* p) {
    if (minHeap->currentSize <= 0) {
        printf("Heap is empty\n");
        return;
    }
    p=getMinPtr(minHeap);
    if (minHeap->currentSize == 1) {
        minHeap->currentSize--;
        return;
    }
    minHeap->arr[0] = minHeap->arr[minHeap->currentSize - 1];
    minHeap->currentSize--;
    MinHeapifySRTN(minHeap, 0);
}

void deleteMinHPF(MinHeap* minHeap) {
    if (minHeap->currentSize <= 0) {
        printf("Heap is empty\n");
        return;
    }
    if (minHeap->currentSize == 1) {
        minHeap->currentSize--;
        return;
    }
    minHeap->arr[0] = minHeap->arr[minHeap->currentSize - 1];
    minHeap->currentSize--;
    MinHeapifyHPF(minHeap, 0);
}