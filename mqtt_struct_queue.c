#include <stdio.h>
#include <stdlib.h>

typedef struct queueElem {
    char *topic;
    char *value;
    struct queueElem *next;
} queueElem;

typedef struct Queue {
    queueElem * head;
    queueElem * tail;
} Queue;

Queue * initQueue() {
 Queue * Q = malloc(sizeof(Queue));
 Q->head = NULL;
 Q->tail = NULL;
 return Q;
}

void enqueue(Queue * Q, char *topic, char *value) {
    queueElem *el = malloc(sizeof(queueElem));
    el->topic = topic;
    el->value = value;
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

void dequeue(Queue * Q, char **topic, char **value) {
    queueElem * out = Q->head;
    Q->head = out->next;
    out->next = NULL;
    *topic = out->topic;
    *value = out->value;
    free(out);
}