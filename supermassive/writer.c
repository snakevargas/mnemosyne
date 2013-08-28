#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define ONE_MEGA (1024 * 1024)


int main() {
  char *shm_name = "lol";

  // Open the shared memory
  int shm_fd = shm_open(shm_name, O_RDWR | O_CREAT, 0777);
  if(shm_fd == -1) {
    fprintf(stderr, "shm_open error: %s\n", strerror(errno));
    exit(-1);
  }
  printf("FD is: %d\n", shm_fd);


  // Make the memory space the right size
  size_t mem_size = sizeof(char) * ONE_MEGA * 5;
  ftruncate(shm_fd, mem_size);


  // Mmap the shared mem
  printf("Size: %lu\n", mem_size);
  void *shared_mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if(shared_mem == (void *) -1) {
    fprintf(stderr, "mmap error: %s\n", strerror(errno));
  }
  printf("Shared mem address is: %p\n", shared_mem);


  // Maybe twiddle some bits directly?


  sleep(10);


  // Close the shared memory
  munmap(shared_mem, mem_size);
  close(shm_fd);
  int status = shm_unlink(shm_name);
  printf("Unlink status: %d\n", status);

  return 0;
}


