#ifndef CIRCULARLIST_H
#define CIRCULARLIST_H
#include "headers.h"


struct Node
{
    struct process data;
    struct Node *next;
    struct Node *prev;
};

struct CircularList
{
    struct Node *head;
    struct Node *tail;
    struct Node *current;
    int size;
};

/**
 * Creates a new circular doubly linked list
 * @return pointer to the list
 */
struct CircularList *createCircularList() {
    struct CircularList *list = (struct CircularList *)malloc(sizeof(struct CircularList));
    if (list == NULL) {
        // Handle memory allocation error
        return NULL;
    }
    list->head = NULL;
    list->tail = NULL;
    list->current = NULL;
    list->size = 0;
    return list;
}

/**
 * Inserts a new node at the beginning of the list
 * @param list pointer to the list
 * @param newData data to be inserted
 */
void insertAtBeginning(struct CircularList *list, struct process newData) {
    struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
    if (newNode == NULL) {
        // Handle memory allocation error
        return;
    }
    newNode->data = newData;
    if (list->head == NULL) {
        newNode->next = newNode;
        newNode->prev = newNode;
        list->head = newNode;
        list->tail = newNode;
        list->current = newNode;
    } else {
        newNode->next = list->head;
        newNode->prev = list->tail;
        list->head->prev = newNode;
        list->tail->next = newNode;
        list->head = newNode;
    }
    list->size++;
}

/**
 * Inserts a new node at the end of the list
 * @param list pointer to the list
 * @param newData data to be inserted
 */
void insertAtEnd(struct CircularList *list, struct process newData) {
    struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
    if (newNode == NULL) {
        // Handle memory allocation error
        return;
    }
    newNode->data = newData;
    if (list->head == NULL) {
        newNode->next = newNode;
        newNode->prev = newNode;
        list->head = newNode;
        list->tail = newNode;
        list->current = newNode;
    } else {
        newNode->next = list->head;
        newNode->prev = list->tail;
        list->head->prev = newNode;
        list->tail->next = newNode;
        list->tail = newNode;
    }
    list->size++;
}
/**
 * @brief Deletes the node at the beginning of the list
 * 
 * @param list pointer to the list
 */
void deleteAtBeginning(struct CircularList *list) {
    if (list->head == NULL) {
        // List is empty
        return;
    }
    if (list->head == list->tail) {
        free(list->head);
        list->head = NULL;
        list->tail = NULL;
        list->current = NULL;
    } else {
        struct Node *temp = list->head;
        list->head = list->head->next;
        list->head->prev = list->tail;
        list->tail->next = list->head;
        free(temp);
    }
    list->size--;
}
/**
 * @brief Deletes the node at the end of the list
 * 
 * @param list pointer to the list
*/
void deleteAtEnd(struct CircularList *list) {
    if (list->head == NULL) {
        // List is empty
        return;
    }
    if (list->head == list->tail) {
        free(list->head);
        list->head = NULL;
        list->tail = NULL;
        list->current = NULL;
    } else {
        struct Node *temp = list->tail;
        list->tail = list->tail->prev;
        list->tail->next = list->head;
        list->head->prev = list->tail;
        free(temp);
    }
    list->size--;
}

/**
 * @brief Checks if the list is empty
 * 
 * @param list pointer to the list
 * @return int 1 if the list is empty, 0 otherwise
 */
int isEmpty(struct CircularList *list) {
    return list->size == 0;
}

/**
 * @brief Returns the size of the list
 * 
 * @param list pointer to the list
 * @return int size of the list
*/
int getSize(struct CircularList *list) {
    return list->size;
}

/**
 * @brief Displays the list
 * 
 * @param list pointer to the list
*/
void displayList(struct CircularList *list) {
    if (list->head == NULL) {
        printf("List is empty\n");
        return;
    }
    struct Node *current = list->head;
    printf("Nodes of the circular doubly linked list: \n");
    do {
        // Print current node data
        struct process p = current->data;
        printf("ID: %d, PID: %d, Arrival Time: %d\n", p.id, p.pid, p.arrivaltime); // Modify this to print your process data
        // Move to next node
        current = current->next;
    } while (current != list->head);
    printf("\n");
}
/**
 * @brief Destroys the list
 * 
 * @param list pointer to the list
*/
void destroyList(struct CircularList *list) {
    if (list->head == NULL) {
        free(list);
        return;
    }
    struct Node *current = list->head;
    struct Node *next;
    do {
        next = current->next;
        free(current);
        current = next;
    } while (current != list->head);
    free(list);
}
/**
 * @brief Changes the current node to the next node
 * 
 * @param list pointer to the list
 */
void changeCurrent(struct CircularList *list) {
    list->current = list->current->next;
}

/**
 * @brief Returns the data of the current node
 * 
 * @param list pointer to the list
 * @param item pointer to the process struct to store the data
 * 
 * @return int 1 if the current node is not NULL, 0 otherwise
*/
int getCurrent(struct CircularList *list, struct process *item) {
    if (list->current == NULL) {
        return 0;
    }
    *item = list->current->data;
    return 1;
}

/**
 * @brief Removes the current node from the list
 * 
 * @param list pointer to the list
 * @param item pointer to the process struct to store the data
 * 
 * @return int 1 if the current node is not NULL, 0 otherwise
*/
int removeCurrent(struct CircularList *list, struct process *item) {
    if (list->current == NULL) {
        return 0;
    }
    *item = list->current->data;
    if (list->current == list->head) {
        list->current = list->current->next;
        deleteAtBeginning(list);
    } else if (list->current == list->tail) {
        list->current = list->current->next;
        deleteAtEnd(list);
    } else {
        struct Node *temp = list->current;
        list->current->prev->next = list->current->next;
        list->current->next->prev = list->current->prev;
        list->current = list->current->next;
        free(temp);
        list->size--;
    }
    return 1;
}

/**
 * @brief Changes the data of the current node
 * 
 * @param list pointer to the list
 * @param item process struct to store the inserted data
 * 
 * @return int 1 if the current node is not NULL, 0 otherwise
*/
int changeCurrentData(struct CircularList *list, struct process item) {
    if (list->current == NULL) {
        return 0;
    }
    list->current->data = item;
    return 1;
}

#endif