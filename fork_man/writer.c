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


// This struct will be used to communicate (2-way) with child processes via shared memory
// Since we'll be using ephemeral shared memory for this struct, everything needs to be staticly allocated
typedef struct comm_t {
  pid_t child_pid;
  char shm_name[32];
  long shm_size;
  time_t last_hb_time;
  int status;
  long response_size;
  char response_name[32];
} comm_t;


// A simple list to keep track of all the children
typedef struct child_info_list_t {
  comm_t *data;
  struct child_info_list_t *next;
} child_info_list_t;


child_info_list_t *insert_child(child_info_list_t *child_list, comm_t *data);
void delete_child(child_info_list_t *child_list, pid_t child_pid);
comm_t *get_child(child_info_list_t *child_list, pid_t child_pid);


void child_logic();


int main() {
  char *control_file = "ctrl_file";
  int ctrl_file;
  int need_open_pipe = 0;

  int requested_hup = 0;
  int rd_buf_size = 100;
  char *read_buffer = (char *) malloc(sizeof(char) * rd_buf_size);
  memset(read_buffer, 0, rd_buf_size);

  child_info_list_t *child_list;
  pid_t child_pids[MAX_NUM_CHILDREN];
  int num_children = 0;
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
    printf("YAY! Polled!\n");
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
        printf("Child pids:\n");
        for(int i = 0; i < num_children; i++) {
          printf("\tChild[%d] : %d\n", i, child_pids[i]);
        }
      }
      if((strncmp(read_buffer, "fork", 4) == 0) && (num_children < MAX_NUM_CHILDREN)) {
        printf("Forking child!\n");

        // First, set up the communications memory
        char child_mem_name[32];
        memset(child_mem_name, 0, 32);
        sprintf(child_mem_name, "ajtest_%d", child_counter);
        child_counter++;
        comm_t *child_shm = (comm_t *) shm_create_map(child_mem_name, sizeof(comm_t));
        strcpy(child_shm->shm_name, child_mem_name);
        child_shm->shm_size = sizeof(comm_t);
        child_list = insert_child(child_list, child_shm);

        pid_t child_pid = fork();
        if(child_pid < 0) {
          fprintf(stderr, "Fork error: %s\n", strerror(errno));
        }
        if(child_pid == 0) {
          // Child process, do child logic
          child_logic();
          exit(0); // Need to exit here so the children don't try polling the fifo pipe
        }
        else {
          // Parent process, save the pid for killing later
          child_shm->child_pid = child_pid;
          child_pids[num_children] = child_pid;
          num_children++;
        }
      }
      if(strncmp(read_buffer, "kill", 4) == 0) {
        printf("Killing children!\n");
        for(int i = 0; i < num_children; i++) {
//          kill(child_pids[i], 15);
        }
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



void child_logic() {
  pid_t my_pid = getpid();
  pid_t parent_pid = getppid();
  printf("My parent is %d and i am %d\n", parent_pid, my_pid);
  sleep(10);
}



child_info_list_t *insert_child(child_info_list_t *child_list, comm_t *data) {
  child_info_list_t *child = (child_info_list_t *) malloc(sizeof(child_info_list_t));
  child->data = data;
  child->next = child_list;
  if(child_list != NULL) {
    child_list->next = child;
  } else {
    child_list = child;
  }
  return child_list;
}



void delete_child(child_info_list_t *child_list, pid_t child_pid) {
  int child_deleted = 0;
  child_info_list_t *curr_child = child_list;
  child_info_list_t *prev_child = NULL;
  while(curr_child != NULL && child_deleted == 0) {
    if(curr_child->data->child_pid == child_pid) {
      // This is the child we're looking for, delete it
      if(prev_child == NULL) {
        child_list = curr_child->next;
      } else {
        prev_child->next = curr_child->next;
      }
      prev_child = curr_child;
      curr_child = curr_child->next;
      shm_unlink_unmap(prev_child->data->shm_name, prev_child->data->shm_size, prev_child->data);
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



