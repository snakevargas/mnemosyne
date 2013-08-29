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
  printf("Size: %d\n", mem_size);
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


/*
void *shm_malloc(void *shared_mem, size_t num_bytes) {
  static size_t last_address = 0;
  last_address += num_bytes;
  return (void *) (shared_mem + last_address - num_bytes);
}
*/

void *shm_malloc(size_t num_bytes) {
  static size_t last_address = 0;
  last_address += num_bytes;
  return last_address - num_bytes;
}




/********************************
* Struct manipulation functions *
********************************/

thingy_t *create_struct(void *shared_mem) {
  // Create the initial struct
  int root_addr = shm_malloc(sizeof(thingy_t));
  thingy_t *the_thingy = (thingy_t *) (shared_mem + root_addr);
  the_thingy->b = shm_malloc(sizeof(thingy_t->b));

  // The struct with offsets exists in the shared memory, now create a usable struct that points into the shared mem
  return load_struct(shared_mem);
}


thingy_t load_struct(void *shared_mem) {
  thingy_t *mapped_thingy = (thingy_t *) malloc(sizeof(thingy_t));
  
}

// struct def for reference
typedef struct thingy_t {
	int a;
	int *b;
} thingy_t;


