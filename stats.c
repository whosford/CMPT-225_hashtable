#include <stdio.h>
#include <stdlib.h>
#include "stats.h"

typedef struct {
    int fnum, made, eaten;
    double min_delay, max_delay, avg_delay;
}FACTORY;

FACTORY* stats = NULL;

int stats_init(int num_producers)
{
    int i, rv = 0;

    stats = (FACTORY *) realloc (stats, num_producers * sizeof(FACTORY));

    if (stats != NULL) {
        for (i = 0; i < num_producers; i++) {  
            stats[i].fnum = i;
            stats[i].made = 0;
            stats[i].eaten = 0;
            stats[i].min_delay = 100000.0;
            stats[i].max_delay = 0.0;
            stats[i].avg_delay = 0.0;
        }
    }
    else
        rv = -1;
        
    return rv;
}

void stats_cleanup(void)
{
    free(stats);

    return;
}

void stats_record_produced(int factory_number)
{
    stats[factory_number].made++;
    return;
}
void stats_record_consumed(int factory_number, double delay_in_ms)
{
    stats[factory_number].eaten++;
    if (delay_in_ms < stats[factory_number].min_delay)
        stats[factory_number].min_delay = delay_in_ms;
    if (delay_in_ms > stats[factory_number].max_delay)
        stats[factory_number].max_delay = delay_in_ms;
    stats[factory_number].avg_delay += delay_in_ms;

    return;
}

void stats_display(int num_producers)
{
    int i;

    printf("%8s%10s%10s%18s%18s%18s\n","Factory#", "#Made", "#Eaten", "Min Delay (ms)", "Max Delay (ms)", "Avg Delay (ms)");
    for (i = 0; i < num_producers; i++) {
        if (stats[i].made != stats[i].eaten) {
            printf("Error: Factory %i, mismatch between number made and eaten.\n", stats[i].fnum);
            break;
        }
        stats[i].avg_delay = stats[i].avg_delay / stats[i].made;
        printf("%4d", stats[i].fnum);
        printf("%12d", stats[i].made);
        printf("%10d", stats[i].eaten);
        printf("%19.5f", stats[i].min_delay);
        printf("%18.5f", stats[i].max_delay);
        printf("%18.5f", stats[i].avg_delay);
        printf("\n");
    }

    return;
}
