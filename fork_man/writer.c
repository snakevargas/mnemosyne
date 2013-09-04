#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>

#include "util.h"


#define MAX_NUM_CHILDREN 10


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
typedef enum {NO_OP, COMMAND_DIE} command_t;
typedef enum {LISTENING, PROCESSING, EXITING, DEAD} status_t;

// This struct will be used to communicate (2-way) with child processes via shared memory
// Since we'll be using ephemeral shared memory for this struct, everything needs to be staticly allocated
typedef struct comm_t {
  pid_t child_pid;
  command_t command;
  command_status_t command_status;
  time_t last_hb_time;
  status_t status;
  long response_size;
  char response_name[32];
} comm_t;


// A simple list to keep track of all the children
typedef struct child_info_list_t {
	char shm_name[32];
	long shm_size;
  comm_t *data;
  struct child_info_list_t *next;
} child_info_list_t;


child_info_list_t *insert_child(child_info_list_t *child_list, comm_t *data);
void delete_whole_list(child_info_list_t *child_list);
void delete_child(child_info_list_t **child_list, pid_t child_pid);
comm_t *get_child(child_info_list_t *child_list, pid_t child_pid);


void print_child_data(child_info_list_t *child_list);
void kill_all_children(child_info_list_t *child_list);
void request_exit_all_children(child_info_list_t *child_list);
void purge_dead_children(child_info_list_t **child_list);


void child_logic(comm_t *comm_mem);


int main() {
  char *control_file = "ctrl_file";
  int ctrl_file;
  int need_open_pipe = 0;

  int requested_hup = 0;
  int rd_buf_size = 100;
  char *read_buffer = (char *) malloc(sizeof(char) * rd_buf_size);
  memset(read_buffer, 0, rd_buf_size);

  child_info_list_t *child_list = NULL;
  int child_counter = 0;


  // Open up the control file, this will be used for external programs to issue commands to the process manager
  ctrl_file = open("ctrl_file", O_RDONLY);
  if(ctrl_file == -1) {
    fprintf(stderr, "open failed on %s: %s\n", control_file, strerror(errno));
    exit(-1);
  }
  struct pollfd x[1];
  x[0].fd = ctrl_file;
  x[0].events = POLLIN | POLLHUP;

  // Main event loop
  while(requested_hup != 1) {
    if(need_open_pipe == 1) {
      ctrl_file = open("ctrl_file", O_RDONLY);
      if(ctrl_file == -1) {
        fprintf(stderr, "open failed on %s: %s\n", control_file, strerror(errno));
        exit(-1);
      }
      need_open_pipe = 0;
    }
    int poll_status = poll(x, 1, -1);
    if(poll_status <= 0) {
      fprintf(stderr, "Poll no good\n");
      exit(-1);
    }
    if(x->events & x->revents == 0) {
      fprintf(stderr, "Events no good: e%d r%d", x->events, x->revents);
      exit(-1);
    }
    if(x->revents & POLLIN != 0) {
      // There are bytes to read
      memset(read_buffer, 0, rd_buf_size);
      int bytes_read = read(x->fd, read_buffer, rd_buf_size);
      printf("Bytes read: %d = *%s*\n", bytes_read, read_buffer);
      // Get the command
      if(strncmp(read_buffer, "die", 3) == 0) {
        printf("Console requested program exit!\n");
        requested_hup = 1;
      }
      if(strncmp(read_buffer, "status", 6) == 0) {
        // Get status, print out the child pids
        printf("Child info:\n");
				print_child_data(child_list);
      }
      if(strncmp(read_buffer, "fork", 4) == 0) {
        printf("Forking child!\n");

        // First, set up the communications memory
        char child_mem_name[32];
        memset(child_mem_name, 0, 32);
        sprintf(child_mem_name, "/ajtest_%d", child_counter);
        child_counter++;
        comm_t *child_shm = (comm_t *) shm_create_map(child_mem_name, sizeof(comm_t));
        child_list = insert_child(child_list, child_shm);
				strcpy(child_list->shm_name, child_mem_name);
        child_list->shm_size = sizeof(comm_t);

        pid_t child_pid = fork();
        if(child_pid < 0) {
          fprintf(stderr, "Fork error: %s\n", strerror(errno));
        }
        if(child_pid == 0) {
          // Child process, do child logic
          child_logic(child_shm);
          exit(0); // Need to exit here so the children don't try polling the fifo pipe
        }
        else {
          // Parent process, save the pid for killing later
          child_shm->child_pid = child_pid;
        }
      }
      if(strncmp(read_buffer, "nice kill", 9) == 0) {
        printf("Sending exit request to children!\n");
        request_exit_all_children(child_list);
      }
      if(strncmp(read_buffer, "kill", 4) == 0) {
        printf("Killing children!\n");
				kill_all_children(child_list);
      }
      if(strncmp(read_buffer, "purge", 5) == 0) {
        printf("Purging dead children from the pid list\n");
        purge_dead_children(&child_list);
      }
    }
    if(x->revents & POLLHUP != 0) {
      // We need to hangup the pipe and reopen it
      need_open_pipe = 1;
    }
  }
  free(read_buffer);

  close(ctrl_file);

  return 0;
}



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
    if(comm_mem->command_status == COMMAND_FROM_PARENT_READY) {
      comm_mem->command_status = COMMAND_IN_USE_BY_CHILD;
      switch(comm_mem->command) {
        case COMMAND_DIE:
          comm_mem->status = EXITING;
          printf("Child %d here, I was requested to exit.\n", comm_mem->child_pid);
          requested_exit = 1;
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



child_info_list_t *insert_child(child_info_list_t *child_list, comm_t *data) {
  child_info_list_t *child = (child_info_list_t *) malloc(sizeof(child_info_list_t));
  child->data = data;
  child->next = child_list;
  child_list = child;
  return child_list;
}



void delete_whole_list(child_info_list_t *child_list) {
	child_info_list_t *curr_child = child_list;
	child_info_list_t *prev_child = NULL;
	while(curr_child != NULL) {
		fprintf(stderr, "Cleaning up %d...", curr_child->data->child_pid);
		prev_child = curr_child;
		curr_child = curr_child->next;
		shm_unlink_unmap(prev_child->shm_name, prev_child->shm_size, prev_child->data);
		fprintf(stderr, "shm clear...");
		free(prev_child);
		fprintf(stderr, "free...\n");
	}
}



void delete_child(child_info_list_t **child_list, pid_t child_pid) {
  int child_deleted = 0;
  child_info_list_t *curr_child = *child_list;
  child_info_list_t *prev_child = NULL;
  while(curr_child != NULL && child_deleted == 0) {
    if(curr_child->data->child_pid == child_pid) {
      // This is the child we're looking for, delete it
      if(prev_child == NULL) {
        *child_list = curr_child->next;
      } else {
        prev_child->next = curr_child->next;
      }
      prev_child = curr_child;
      curr_child = curr_child->next;
			if(prev_child->data != NULL) {
	      shm_unlink_unmap(prev_child->shm_name, prev_child->shm_size, prev_child->data);
			}
      free(prev_child);
      child_deleted = 1;
    } else {
      prev_child = curr_child;
      curr_child = curr_child->next;
    }
  }
}



comm_t *get_child(child_info_list_t *child_list, pid_t child_pid) {
  child_info_list_t *curr_child = child_list;
  while(curr_child != NULL) {
    if(curr_child->data->child_pid == child_pid) {
      return curr_child->data;
    }
  }
  return NULL;
}



void print_child_data(child_info_list_t *child_list) {
	child_info_list_t *curr_child = child_list;
	while(curr_child != NULL) {
		printf("Child pid: %d\n", curr_child->data->child_pid);
		printf("\tshm_name: %s\n", curr_child->shm_name);
		printf("\tshm_size: %ld\n", curr_child->shm_size);
		printf("\tlast_hb: %ld\n", curr_child->data->last_hb_time);
    switch(curr_child->data->status) {
      case LISTENING:
    		printf("\tstatus: LISTENING\n");
        break;
      case PROCESSING:
    		printf("\tstatus: PROCESSING\n");
        break;
      case EXITING:
    		printf("\tstatus: EXITING\n");
        break;
      case DEAD:
    		printf("\tstatus: DEAD\n");
        break;
      default:
        break;
    }
		printf("\tdata: %p\n", curr_child->data);
		curr_child = curr_child->next;
	}
}



void kill_all_children(child_info_list_t *child_list) {
	child_info_list_t *curr_child = child_list;
	while(curr_child != NULL) {
		kill(curr_child->data->child_pid, 9);
		curr_child = curr_child->next;
	}
	printf("Child processes killed. Cleaning up\n");
	// Delete all child nodes
	delete_whole_list(child_list);
}



void request_exit_all_children(child_info_list_t *child_list) {
  child_info_list_t *curr_child = child_list;
  while(curr_child != NULL) {
    if(curr_child->data->command_status == NO_COMMAND) {
      curr_child->data->command = COMMAND_DIE;
      curr_child->data->command_status = COMMAND_FROM_PARENT_READY;
    }
    curr_child = curr_child->next;
  }
}



void purge_dead_children(child_info_list_t **child_list) {
  child_info_list_t *curr_child = *child_list;
  while(curr_child != NULL) {
    if(curr_child->data->status == DEAD) {
      delete_child(child_list, curr_child->data->child_pid);
    }
    curr_child = curr_child->next;
  }
}




