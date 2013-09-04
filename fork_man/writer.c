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
  time_t last_hb_time;
  int status;
  long response_size;
  char response_name[32];
} comm_t;




void child_logic();


int main() {
  char *control_file = "ctrl_file";
  int ctrl_file;
  int need_open_pipe = 0;

  int requested_hup = 0;
  int rd_buf_size = 100;
  char *read_buffer = (char *) malloc(sizeof(char) * rd_buf_size);
  memset(read_buffer, 0, rd_buf_size);

  pid_t child_pids[MAX_NUM_CHILDREN];
  int num_children = 0;


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

/*
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

*/


void child_logic() {
  pid_t my_pid = getpid();
  pid_t parent_pid = getppid();
  printf("My parent is %d and i am %d\n", parent_pid, my_pid);
  sleep(10);
}

