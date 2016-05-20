#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include "bbuff.h"

pthread_mutex_t lock;
sem_t full, empty;
int counter;
void* buffer[BUFFER_SIZE];

void bbuff_init()
{
    pthread_mutex_init(&lock, NULL);
    sem_init(&full, 0, 0);
    sem_init(&empty, 0, BUFFER_SIZE);
    counter = 0;

    return;
}

void bbuff_blocking_insert(void* item)
{
    sem_wait(&empty);
    pthread_mutex_lock(&lock);
    if (counter < BUFFER_SIZE) {
        buffer[counter] = item;
        counter++;
        pthread_mutex_unlock(&lock);
        sem_post(&full);
        return;
   }
   else {
        pthread_mutex_unlock(&lock);
        sem_post(&full);
        return;
   }
}

void* bbuff_blocking_extract()
{
    void* item;
    sem_wait(&full);
    pthread_mutex_lock(&lock);
    if (counter > 0) {
        item = buffer[counter - 1];
        counter--;
        pthread_mutex_unlock(&lock);
        sem_post(&empty);
        return item;
    }
    else {
        pthread_mutex_unlock(&lock);
        sem_post(&empty);
        return NULL;
    }
}

_Bool bbuff_is_empty()
{
    if (counter == 0)
        return true;
    else
        return false;
}