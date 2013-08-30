#ifndef __AMIT_THINGY_H__
#define __AMIT_THINGY_H__


typedef struct thingy_t {
	int a;
	int b;
	char c[7];
	int d;
} thingy_t;


thingy_t *create_thingy();
void destroy_thingy(thingy_t *the_thingy);
void print_thingy(thingy_t *the_thingy);


#endif

