#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

void parent_logic(void *shared_mem);
void child_logic(void *shared_mem);


char *mem_name = "lol";
size_t mem_size = sizeof(char) * ONE_MEGA * 5;


int main() {

  // Create the shared memory
  void *shared_mem = shm_create_map(mem_name, mem_size);


  // Fork out the child processes
  pid_t child_pid = fork();
  if(child_pid < 0) {
    fprintf(stderr, "Fork error: %s\n", strerror(errno));
  }

  if(child_pid > 0) {
    // Save the child pid for later, so we can issue commands to the children
    // TODO: err.. do this later, whatever
    // Do parent code here
    parent_logic(shared_mem);
  }
  if(child_pid == 0) {
    // This is the child process, run child code!
    child_logic(shared_mem);
  }

  // Now fork off a bunch of children
  for(int i = 0; i < 4; i++) {
    pid_t child_pid = fork();
    if(child_pid < 0) {
      fprintf(stderr, "Fork error: %s\n", strerror(errno));
    }
    else if(child_pid == 0) {
      child_logic(shared_mem);
    }
  }


  return 0;
}



void parent_logic(void *shared_mem) {
  // Create the struct
  thingy_t *thingy = create_struct(shared_mem);

  // Set initial values
  thingy->a = 42;
  *thingy->b = 13;

  printf("Parent:\n");
  print_struct(thingy);

  // Sleep for a while
  sleep(4);

  // Twiddle some bits
  thingy->a++;
  *thingy->b = 512;

  printf("Parent:\n");
  print_struct(thingy);

  // Sleep some more
  sleep(4);

  // Done!
  int status = shm_unlink_unmap(mem_name, mem_size, shared_mem);
}


void child_logic(void *shared_mem) {
  sleep(2);

  thingy_t *thingy = (thingy_t *) shared_mem;

  // Print out the struct!
  printf("Child:\n");
  print_struct(thingy);

  // Sleep for a while
  sleep(4);

  // Print it out again!
  printf("Child:\n");
  print_struct(thingy);

  // Done!
}



