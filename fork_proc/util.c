#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "util.h"


void *shm_create_map(char *shm_name, size_t mem_size) {
  // Open the shared memory
  int shm_fd = shm_open(shm_name, O_RDWR | O_CREAT, 0777);
  if(shm_fd == -1) {
    fprintf(stderr, "shm_open error: %s\n", strerror(errno));
    exit(-1);
  }
  printf("FD is: %d\n", shm_fd);

  // Make the memory space the right size
  ftruncate(shm_fd, mem_size);

  // Mmap the shared mem
  printf("Size: %hu\n", (unsigned int) mem_size);
  void *shared_mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if(shared_mem == (void *) -1) {
    fprintf(stderr, "mmap error: %s\n", strerror(errno));
  }
  printf("Shared mem address is: %p\n", shared_mem);

  // It is safe to close the shm file descriptor after mmap'ing the memory
  close(shm_fd);

  return shared_mem;
}


int shm_unlink_unmap(char *mem_name, size_t mem_size, void *shared_mem) {
  munmap(shared_mem, mem_size);
  int status = shm_unlink(mem_name);
  printf("Unlink status: %d\n", status);
  return status;
}


void *shm_malloc(void *shared_mem, size_t num_bytes) {
  static size_t last_address = 0;
  last_address += num_bytes;
  return (void *) (shared_mem + last_address - num_bytes);
}



/********************************
* Struct manipulation functions *
********************************/

thingy_t *create_struct(void *shared_mem) {
  // Create the struct
  thingy_t *thingy = (thingy_t *) shm_malloc(shared_mem, sizeof(thingy_t));
  thingy->b = (int *) shm_malloc(shared_mem, sizeof(int));
  return thingy;
}


void print_struct(thingy_t *thingy) {
  printf("Thingy: %p\n", thingy);
  printf("\tthingy->a: %p = %d\n", &thingy->a, thingy->a);
  printf("\tthingy->b: %p = %d\n", thingy->b, *thingy->b);
}


