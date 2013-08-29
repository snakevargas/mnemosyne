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

  // Open the shared memory
  void *shared_mem = shm_create_map(mem_name, mem_size);

  thingy_t *thingy = load_struct(shared_mem);

/*
  int *x = (int *) shm_malloc(shared_mem, sizeof(int));
  int **next = (int **) shm_malloc(shared_mem, sizeof(int *));
  int *y = (int *) shm_malloc(shared_mem, sizeof(int));
  int address1 = 0;
  int *x = (int *) shared_mem;
  int address2 = address1 + sizeof(int);
  int *y = (int *) (shared_mem + address2);
*/

  // Print it out!
  print_struct(thingy);
/*
  printf("X:%p = %d\n", x, *x);
  printf("Next:%p = %p\n", next, *next);
  printf("Y:%p = %d\n", y, *y);
  printf("**Next %d\n", **next);
*/


  sleep(1);

  // Close the shared memory or maybe don't, since we're the client
//  int status = shm_unlink_unmap(mem_name, mem_size, shared_mem);


  return 0;
}


