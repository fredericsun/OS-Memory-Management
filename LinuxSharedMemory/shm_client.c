#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
// TODO(student): Include necessary headers
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>

#define SHM_NAME "yues_yangl"
#define PAGE_SIZE 4096

sem_t * mutex;
char *addr;
struct timeval timebefore;
struct timeval timeafter;

typedef struct {
  int pid;
  char birth[25];
  int elapsed_sec;
  double elapsed_msec;
  int v_bit;
} stats_t;

stats_t *p_current;

void exit_handler(int sig) {
    // new routine defined here specified by sigaction() in main
    // TODO(student): critical section begins
    sem_wait(mutex);
    // client reset its segment, or mark its segment as valid
    // so that the segment can be used by another client later.
    p_current -> v_bit = 0;
    sem_post(mutex);
    if (munmap(addr, PAGE_SIZE) == -1) {
        fprintf(stderr, "munmap() failed.\n");
        exit(1);
    }
    // critical section ends
    exit(0);
}

int main(int argc, char *argv[]) {
    // TODO(student): Signal handling
    // Use sigaction() here and call exit_handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = exit_handler;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
      fprintf(stderr, "SIGINT error.\n");
      exit(1);
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
      fprintf(stderr, "SIGTERM error.\n");
      exit(1);
    }
    // TODO(student): Open the preexisting shared memory segment created by
    // shm_server
    int fd_shm = shm_open(SHM_NAME, O_RDWR, 0660);
    addr = (char *)mmap(0, PAGE_SIZE,
    PROT_READ|PROT_WRITE, MAP_SHARED, fd_shm, 0);
    if (addr == MAP_FAILED)
      exit(1);
    // TODO(student): point the mutex to the particular segment of the shared
    // memory page
    mutex = (sem_t *)addr;
    stats_t *p_struct =(stats_t *)((void *) addr + sizeof(sem_t) * 2);
    // TODO(student): critical section begins
    sem_wait(mutex);
        // client searching through the segments of the page to find a valid
        // (or available) segment and then mark this segment as invalid
    for (int i = 0; i < 63; i++) {
      stats_t *p = (stats_t *)((void *)p_struct + i * sizeof(stats_t));
      p_current = p;
      if (p_current -> v_bit == 0) {
        p_current -> pid = getpid();
        time_t curtime;
        time(&curtime);
        char *ctime_string = ctime(&curtime);
        strncpy(p_current -> birth, ctime_string, strlen(ctime_string) - 1);
        gettimeofday(&timebefore, NULL);
        p_current -> v_bit = 1;
        sem_post(mutex);
        goto LOOP;
      }
    }
    sem_post(mutex);
    exit(1);
    // critical section ends

    LOOP: while (1) {
      // TODO(student): fill in fields in stats_t
      gettimeofday(&timeafter, NULL);
      p_current -> elapsed_sec = (int)(timeafter.tv_sec - timebefore.tv_sec);
      p_current -> elapsed_msec = (timeafter.tv_usec -
      timebefore.tv_usec) / 1000.0f;
      sleep(1);
      // display the PIDs of all the active clients
      printf("Active clients :");
      for (int i = 0; i < 63; i++) {
        stats_t *p_current =(stats_t *)((void *)p_struct + i * sizeof(stats_t));
        if (p_current -> v_bit == 1) {
          int cur_pid = p_current -> pid;
          printf(" %d", cur_pid);
        }
      }
      printf("\n");
    }

    return 0;
}
