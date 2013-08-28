#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "util.h"


int main() {
  char *mem_name = "lol";
  size_t mem_size = sizeof(char) * ONE_MEGA * 5;

  // Create the shared memory
  void *shared_mem = shm_create_map(mem_name, mem_size);

  // "malloc" 2 ints at the beginning
  int *x = (int *) shm_malloc(shared_mem, sizeof(int));
  int **next = (int **) shm_malloc(shared_mem, sizeof(int *));
  int *y = (int *) shm_malloc(shared_mem, sizeof(int));
  *x = 42;
  *next = y;
  *y = 13;

  // Print it out!
  printf("X:%p = %d\n", x, *x);
  printf("Next:%p = %p\n", next, *next);
  printf("Y:%p = %d\n", y, *y);
  printf("**Next %d\n", **next);

  // Maybe twiddle some bits directly?


  // Sleep so the other process can do some stuff
  sleep(10);


  // Close the shared memory
  int status = shm_unlink_unmap(mem_name, mem_size, shared_mem);

  return 0;
}


