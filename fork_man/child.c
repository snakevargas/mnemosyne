#include "includes.h"

#include "util.h"

#include "child.h"


void child_logic(comm_t *comm_mem) {
  int requested_exit = 0;
  pid_t my_pid = getpid();
  pid_t parent_pid = getppid();
  printf("[CHILD] My parent is %d and i am %d\n", parent_pid, my_pid);
  comm_mem->status = LISTENING;
  while(requested_exit == 0) {
    // Heartbeat!
    time(&comm_mem->last_hb_time);

    // Check to see if there is a command from the parent
    if(comm_mem->status == WORK_VALIDATED) {
      comm_mem->status = LISTENING;
    }
    if(comm_mem->command_status == COMMAND_FROM_PARENT_READY) {
      comm_mem->command_status = COMMAND_IN_USE_BY_CHILD;
      switch(comm_mem->command) {
        case COMMAND_DIE:
          comm_mem->status = EXITING;
          printf("Child %d here, I was requested to exit.\n", comm_mem->child_pid);
          requested_exit = 1;
          break;
        case COMMAND_PROCESS_INPUT:
          comm_mem->status = PROCESSING;
          // Open up the shared memory for the input
          char *request_shm = (char *) shm_create_map(comm_mem->request_name, comm_mem->request_size);
          // Do the work
          char *response_string = (char *) malloc(comm_mem->request_size + 100);
          memset(response_string, 0, comm_mem->request_size + 100);
          sprintf(response_string, "%s out: BLAH BLAH BLAH!", request_shm);

          // Now open up the response shared memory
          sprintf(comm_mem->response_name, "child_%d_response", comm_mem->child_pid);
          comm_mem->response_size = strlen(response_string);
          char *response_shm = (char *) shm_create_map(comm_mem->response_name, comm_mem->response_size);
          memcpy(response_shm, response_string, comm_mem->response_size);
          sleep(5); // Simulate difficult work! :P
          comm_mem->status = FINISHED_WORK;
          // Signal the parent (being lazy, just use the ctrl_file fifo)
          system("echo \"work finished\" > ctrl_file");
          break;
        default:
          break;
      }
      comm_mem->command_status = NO_COMMAND;
    }

    sleep(1); // Just to keep the thread from spinning too fast and eating ALL THE CPU
  }
  comm_mem->status = DEAD;
}


