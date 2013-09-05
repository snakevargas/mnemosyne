#ifndef __FORK_PROC_MANAGER_H__
#define __FORK_PROC_MANAGER_H__

#include <time.h>


/*
Operations:
  fork new worker
  kill specific worker
  kill all workers
  get healthcheck of workers? (maybe rolled into the commands stuff below)
  issue specific command to specific worker
  issue specific command to all workers
*/

// Need way for external process to issue commands to the manager
// use a fifo file for now?


typedef enum {NO_COMMAND, COMMAND_FROM_PARENT_READY, COMMAND_IN_USE_BY_CHILD} command_status_t;
typedef enum {NO_OP, COMMAND_PROCESS_INPUT, COMMAND_DIE} command_t;
typedef enum {LISTENING, PROCESSING, FINISHED_WORK, WORK_VALIDATED, EXITING, DEAD} status_t;

// This struct will be used to communicate (2-way) with child processes via shared memory
// Since we'll be using ephemeral shared memory for this struct, everything needs to be staticly allocated
typedef struct comm_t {
  pid_t child_pid;
  command_t command;
  command_status_t command_status;
  time_t last_hb_time;
  status_t status;

  long request_size;
  char request_name[32];
  long response_size;
  char response_name[32];
} comm_t;


// A simple list to keep track of all the children
typedef struct child_info_list_t {
	char shm_name[32];
	long shm_size;
  comm_t *data;
  char *request_shm; // For clean up purposes only
  struct child_info_list_t *next;
} child_info_list_t;







#endif

