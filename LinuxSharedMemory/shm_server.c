#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
// TODO(student): Include necessary headers
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>

#define SHM_NAME "yues_yangl"
#define PAGESIZE 4096
typedef struct {
    int pid;
    char birth[25];
    int elapsed_sec;
    double elapsed_msec;
    int v_bit;
} stats_t;

sem_t * mutex;
char *addr;

void exit_handler(int sig) {
    // TODO(student): Clear the shared memory segment
    if (munmap(addr, PAGESIZE) == -1)
      exit(1);
    if (shm_unlink(SHM_NAME) == -1)
      exit(1);
    exit(0);
}

int main(int argc, char **argv) {
    // TODO(student): Signal handling
    // Use sigaction() here and call exit_handler
    struct sigaction sa;
    memset(&sa, 0 , sizeof(sa));
    sa.sa_handler = exit_handler;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
      perror("SIGINT error\n");
      exit(1);
    }

    // TODO(student): Create a new shared memory segment
    int fd_shm = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0660);
    if (ftruncate(fd_shm, PAGESIZE) == -1)
      exit(1);
    addr = (char *)mmap(0, PAGESIZE,
    PROT_READ|PROT_WRITE, MAP_SHARED, fd_shm, 0);
    if (addr == MAP_FAILED)
      exit(1);
    // TODO(student): Point the mutex to the segment of the shared memory page
    mutex = (sem_t *)addr;
    // that you decide to store the semaphore
    if (sem_init(mutex, 1, 1) == -1)  // Initialize semaphore
      exit(1);
    // TODO(student): Some initialization of the rest of the segments in the
    // shared memory page here if necessary
    stats_t *p_struct =(stats_t *)((void *) addr + sizeof(sem_t) * 2);

    while (1) {
        // TODO(student): Display the statistics of active clients, i.e. valid
        // segments after some formatting
        sleep(1);
        for (int i = 0; i < 63; i++) {
          stats_t *p_current = (stats_t *)((void *)p_struct +
          i * sizeof(stats_t));
          if (p_current -> v_bit != 0)
            printf("pid : %d, birth : %s, elapsed : %d s %f ms\n",
                   p_current -> pid, p_current -> birth,
                   p_current -> elapsed_sec, p_current -> elapsed_msec);
        }
    }
    return 0;
}
