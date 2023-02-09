#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>

int maxItems = 5;
int count = 0;
bool full;
int items[5];

void put() {
}

void put(sem_t *mySem, int number) {
 if(count == maxItems ) {
  sem_wait(mySem);
 }
 count += 1;
 if(count == 1) {
  full = false;
  sem_post(mySem);
 }
}

int get(sem_t *mySem) {
 if(count == 0) { 
    sem_wait(mySem);
 }
}

int main() {
 pthread_t threadId = (pthread_t) malloc(sizeof(pthread_t));
 sem_t *mySem = (sem_t*) malloc(sizeof(sem_t));
 int semInit = sem_init(mySem, 0, 0);
 if(semInit == 0) {
  printf("Semaphore init success...\n");
 }
 else {
  printf("Semaphore init failed...\n");
  return EXIT_FAILURE;
 }
 // FIXME thread starten, wert in sema sperren, speichern, freigeben, thread schließen
 return EXIT_SUCCESS;
}