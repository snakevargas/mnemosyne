#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "thingy.h"



int main() {
  char *shm_name = "lol";

  // Open the shared memory
  int shm_fd = shm_open(shm_name, O_RDWR | O_CREAT, 0777);
  if(shm_fd == -1) {
    fprintf(stderr, "shm_open error: %s\n", strerror(errno));
    exit(-1);
  }
  printf("FD is: %d\n", shm_fd);


  // Create our shared struct
  printf("Create!\n");
  thingy_t *the_thingy = create_thingy();

  // Print it
  printf("Print!\n");
  print_thingy(the_thingy);


  // Make the memory space the right size
  size_t thingy_size = sizeof(*the_thingy);
  ftruncate(shm_fd, thingy_size);

  // Mmap the shared mem
  printf("Size: %lu\n", thingy_size);
  void *shared_thingy = mmap(NULL, thingy_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if(shared_thingy == (void *) -1) {
    fprintf(stderr, "mmap error: %s\n", strerror(errno));
  }
  printf("Shared thingy address is: %p\n", shared_thingy);

  // Do the memcpy
  memcpy(shared_thingy, the_thingy, thingy_size);

  // Maybe twiddle some bits directly?
  ((thingy_t *) shared_thingy)->b = 13;

  sleep(10);


  // Destroy it
  printf("Destroy!\n");
  destroy_thingy(the_thingy);


  // Close the shared memory
  munmap(shared_thingy, thingy_size);
  close(shm_fd);
  int status = shm_unlink(shm_name);
  printf("Unlink status: %d\n", status);

  return 0;
}


