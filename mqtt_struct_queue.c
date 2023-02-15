#include <stdio.h>
#include <stdlib.h>

// Code auf Basis von Youtube Tutorial
// https://www.youtube.com/watch?time_continue=543&v=yKNPFKfnlt8
// https://www.geeksforgeeks.org/queue-linked-list-implementation/

typedef struct queueElem {
 char *topic, *value;
 struct queueElem *next;
} queueElem;

typedef struct Queue {
 int elements;
 queueElem * head, * tail;
} Queue;

Queue * initQueue() {
 Queue * Q = malloc(sizeof(Queue));
 Q->elements = 0;
 Q->head = Q->tail = NULL;
 return Q;
}

void enqueue(Queue * Q, char *topic, char *value) {
 queueElem *el = malloc(sizeof(queueElem));
 el->topic = topic;
 el->value = value;
 el->next = NULL;
 if(Q->tail == NULL) {
  Q->head = Q->tail = el;  
 }
 else {
  Q->tail->next = Q->tail = el;
 }
 Q->elements++;
}

void dequeue(Queue * Q, char **topic, char **value) {
 if(Q->head == NULL) {
  return;
 }
 queueElem * out = Q->head;
 Q->head = Q->head->next;
 if(Q->head == NULL) {
  Q->tail = NULL;
 }
 *topic = out->topic;
 *value = out->value;
 free(out);
 Q->elements--;
}

int getElementCount(Queue * Q) {
 return Q->elements;
}