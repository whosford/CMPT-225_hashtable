#define _POSIX_C_SOURCE 199309L

#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "bbuff.h"
#include "stats.h"

int factories, kids, seconds;
_Bool stop_thread = false;

typedef struct {
    int factory_number;
    double time_stamp_in_ms;
}candy_t;

double current_time_in_ms(void)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return now.tv_sec * 1000.0 + now.tv_nsec/1000000.0;
}

void *producer(void *param) {
   int fac_num = *((int*) param);
   int wait_time;
   int min = 0, max = 3;

   while(!stop_thread) {
        wait_time = min + rand() / (RAND_MAX / (max - min + 1) + 1);

        printf("Factory %i ships candy and waits %i s.\n", fac_num, wait_time);

        candy_t *candy  = (candy_t *) malloc (sizeof(candy_t));
        if (candy != NULL) {
            candy->factory_number = fac_num;
            candy->time_stamp_in_ms = current_time_in_ms();
        }
        else {
            printf("Error: Producer allocation failed.\n");
            break;
        }
        stats_record_produced(candy->factory_number);
        bbuff_blocking_insert(candy);
        sleep(wait_time);
   }

   printf("Candy factory %i done.\n", fac_num);
   return NULL;
}

void *consumer() 
{
    int wait_time;
    int min = 0, max = 1;
    double delay_in_ms;
    candy_t *candy;

    while (1) {
        wait_time = min + rand() / (RAND_MAX / (max - min + 1) + 1);
        candy = bbuff_blocking_extract();
        if (candy != NULL) {
            delay_in_ms = current_time_in_ms() - candy->time_stamp_in_ms;
            stats_record_consumed(candy->factory_number, delay_in_ms);
            free(candy);
        }
        
        sleep(wait_time);
   }
}

int main(int argc, char* argv[])
{
    if (argc != 4) {
        printf("Error: invalid number of arguments entered.\n");
        return 1;
    }

    factories = atoi(argv[1]);
    kids = atoi(argv[2]);
    seconds = atoi(argv[3]);

    if (factories < 1 || kids < 1 || seconds < 1) {
        printf("Error: all arguments must be greater than 0.\n");
        return 1;
    }

    bbuff_init();
    int rv = stats_init(factories);
    if (rv == -1) {
        printf("Error: Factory allocation failed.\n");
        return 1;
    }   

    int i;
    int f_number[factories];
    pthread_t producers[factories];
    pthread_t consumers[kids]; 

    for (i = 0; i < factories; i++) {
        f_number[i] = i;
        pthread_create(&(producers[i]),NULL,producer,&(f_number[i]));
    }   
    for (i = 0; i < kids; i++) 
        pthread_create(&(consumers[i]),NULL,consumer,NULL);

    i = 0;
    while (i <= seconds) {
        printf("Time %i s\n", i);
        i++;
        sleep(1);
    }
    stop_thread = true;
    for (i = 0; i < factories; i++)
        pthread_join(producers[i],NULL);

    while (!bbuff_is_empty()) {
        printf("Waiting for all candy to be consumed.\n");
        sleep(1);
    }
    for (i = 0; i < kids; i++) {
        pthread_cancel(consumers[i]);
        pthread_join(consumers[i],NULL);
    }

    stats_display(factories);
    stats_cleanup();

    return 0;
}