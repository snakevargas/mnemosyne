#ifndef __AMIT_UTIL_H__
#define __AMIT_UTIL_H__

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>


#define ONE_MEGA (1024 * 1024)

typedef struct thingy_t {
	int a;
	int *b;
} thingy_t;


void *shm_create_map(char *mem_name, size_t mem_size);
int shm_unlink_unmap(char *mem_name, size_t mem_size, void *shared_mem);

void *shm_malloc(void *shared_mem, size_t num_bytes);

thingy_t *create_struct();


#endif

