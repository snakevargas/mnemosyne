#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "thingy.h"


thingy_t *create_thingy() {
  // Malloc the memory
  thingy_t *the_thingy = (thingy_t *) malloc(sizeof(thingy_t));
/*
  the_thingy->a = (int *) malloc(sizeof(int));
  the_thingy->b = (int *) malloc(sizeof(int));
  the_thingy->c = (char *) malloc(sizeof(char) * 7);
  the_thingy->d = (int *) malloc(sizeof(int));
*/

  // Set the fields
  fprintf(stderr, "Setting thingy fields\n");
  the_thingy->a = 42;
  the_thingy->b = 43;
  memset(the_thingy->c, 0, 7);
  strcpy(the_thingy->c, "abc123");
  the_thingy->d = 44;
  return the_thingy;
}


void destroy_thingy(thingy_t *the_thingy) {
/*
  free(the_thingy->a);
  free(the_thingy->b);
  free(the_thingy->c);
  free(the_thingy->d);
*/
  free(the_thingy);
}


void print_thingy(thingy_t *the_thingy) {
  fprintf(stderr, "Thingy!\n");
  fprintf(stderr, "\ta = %d\n", the_thingy->a);
  fprintf(stderr, "\tb = %d\n", the_thingy->b);
  fprintf(stderr, "\tc = %s\n", the_thingy->c);
  fprintf(stderr, "\td = %d\n", the_thingy->d);
}


