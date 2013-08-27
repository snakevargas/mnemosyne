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
  int shm_fd = shm_open(shm_name, O_RDWR | O_CREAT, 0666);
  if(shm_fd == -1) {
    fprintf(stderr, "Error: %s\n", strerror(errno));
    exit(-1);
  }
  printf("FD is: %d\n", shm_fd);


  // Truncate to the right size
  size_t thingy_size = sizeof(thingy_t);
  ftruncate(shm_fd, thingy_size);

  // Mmap the shared mem
  printf("Size: %lu\n", thingy_size);
  void *shared_thingy = mmap(NULL, thingy_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  printf("Shared thingy address is: %p\n", shared_thingy);

  print_thingy(shared_thingy);

  sleep(1);



  // Close the shared memory
  close(shm_fd);

  return 0;
}


