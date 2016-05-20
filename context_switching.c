#define _POSIX_C_SOURCE 199309L
#define _GNU_SOURCE

#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <pthread.h>  

int num = 0, switch_count = 0;
int child_status;
float thread_switching_times[1000];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;

double timespec_to_ns(struct timespec *ts)
{
    return ts->tv_nsec;
}

void func_call()
{
  int i;
  struct timespec start_time, end_time;

  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);
  for(i = 0; i < 5000000; i++);
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
  printf("Cost of minimal function call: %f ns\n", (timespec_to_ns(&end_time) - timespec_to_ns(&start_time)) / 5000000.0);

  return;
}

void sys_call()
{
  int i;
  struct timespec start_time, end_time;

  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);
  for(i = 0; i < 5000000; i++)
    syscall(SYS_getpid);
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
  printf("Cost of minimal system call: %f ns\n", (timespec_to_ns(&end_time) - timespec_to_ns(&start_time)) / 5000000.0);

  return;
}

void process_switching()
{
  int retval, n;
  struct timespec start_time, end_time;
  int parent_to_child[2];
  int child_to_parent[2];
  char buf[2];
  char *msg = "a";
  float process_switch_times[500];
  float avg_time;
  pid_t childpid;

  if (pipe(parent_to_child) == -1) {
    perror("pipe");
    exit(0);
  }
  if (pipe(child_to_parent) == -1) {
    perror("pipe");
    exit(0);
  }

  childpid = fork();

  if (childpid >= 0) {
    if (childpid == 0) {
      for (n = 0; n < 500; n++) {
        close(parent_to_child[1]);
        close(child_to_parent[0]);
        retval = read(parent_to_child[0], buf, 1);
        if (retval == -1) {
          perror("Error reading in child");
          exit(0);
        }
        retval = write(child_to_parent[1], msg, strlen(msg));
        if (retval == -1) {
          perror("Error writing in child");
          exit(0);
        }
      }
      if (close(child_to_parent[1]) == -1)
        perror("close");
      exit(0);  
    }
    else {
      for (n = 0; n < 500; n++) {
        close(parent_to_child[0]);
        close(child_to_parent[1]);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);
        retval = write(parent_to_child[1], msg, strlen(msg));
        if (retval == -1) {
          perror("Error writing in parent");
          break;
        }
        retval = read(child_to_parent[0], buf, 1);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
        process_switch_times[n] = timespec_to_ns(&end_time) - timespec_to_ns(&start_time);
        if (retval == -1) {
          perror("Error reading in parent");
          break;
        }
      }
      if (close(parent_to_child[1]) == -1) {
        perror("close");
        exit(0);
      }
      waitpid(childpid, &child_status, 0); 
      for (n = 0; n < 500; n++)
        avg_time += process_switch_times[n];
      printf("Cost of process switching: %f ns\n", avg_time / 1000.0);
    }
  }
  else {
    perror("fork");
    exit(0);
  }

  return;
}

void *thread1()
{
  int i;
  struct timespec start_time, end_time;

  for (i = 0; i < 500; i++) {
    pthread_mutex_lock(&lock);
    while (num == 0) {
      pthread_cond_wait(&cond1, &lock);
      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
      thread_switching_times[switch_count] = timespec_to_ns(&end_time) - timespec_to_ns(&start_time);
      switch_count++;
    } 
    num = 0;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);
    pthread_cond_signal(&cond2);
    pthread_mutex_unlock(&lock);
  }
    
  return NULL;
}

void *thread2()
{
  int i;
  struct timespec start_time, end_time;

  for (i = 0; i < 500; i++) {
    pthread_mutex_lock(&lock);
    while (num == 1) {
      pthread_cond_wait(&cond2, &lock);
      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
      thread_switching_times[switch_count] = timespec_to_ns(&end_time) - timespec_to_ns(&start_time);
      switch_count++;
    }  
    num = 1;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);
    pthread_cond_signal(&cond1);
    pthread_mutex_unlock(&lock);
  }
    
  return NULL;
}

void thread_switching()
{
  int i;
  float avg_time;
  pthread_t t1, t2;

  pthread_create(&t1, NULL, thread1, NULL);
  pthread_create(&t2, NULL, thread2, NULL);

  pthread_join(t1,NULL);
  pthread_join(t2,NULL);

  for (i = 0; i < switch_count; i++)
    avg_time += thread_switching_times[i];

  printf("Cost of thread switching: %f ns\n", avg_time / switch_count);

  return;
}

int main()
{
  func_call();
  sys_call();
  process_switching();
  thread_switching();

  return 0;
}
