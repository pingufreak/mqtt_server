#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int value;
    struct queueElem *next;
} queueElem;

typedef struct {
    queueElem * head;
    queueElem * tail;
} Queue;

Queue * initQueue() {
 Queue * Q = malloc(sizeof(Queue));
 Q->head = NULL;
 Q->tail = NULL;
 return Q;
}

void enqueue(Queue * Q, int a) {
    queueElem * el = malloc(sizeof(queueElem));
    el->value = a;
    el->next = NULL;
    if(Q->head == NULL && Q->tail == NULL) {
        Q->head=el;
        Q->tail=el;
    }
    else {
        Q->tail->next = el;
        Q->tail = el;
    }
}

int dequeue(Queue * Q) {
    if(Q->head == NULL && Q->tail == NULL) {
        return -1;
    }
    else {
        queueElem * out = Q->head;
        Q->head = out->next;
        out->next = NULL;
        int ausgabe = out->value;
        free(out);
        return ausgabe;
    }
}

void printQueue(Queue * Q) {
    queueElem * temp = Q->head;
    printf("queue:\n");
    while(temp!= NULL) {
        printf("%d\n", temp->value);
        temp = temp->next;
    }
}
void deleteQueue(Queue * Q) {
    while(Q->head != NULL) {
        dequeue(Q);
    }
    free(Q);
}

int main() {
    Queue * queue = initQueue();
    enqueue(queue, 10);
    enqueue(queue, 11);
    enqueue(queue, 12);
    printQueue(queue);
    dequeue(queue);
    dequeue(queue);
    dequeue(queue);
    printQueue(queue);
    deleteQueue(queue);
    return 0;
}