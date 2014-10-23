#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "gtthread.h"

/* Tests creation.
   Should print "Hello World!" */

static gtthread_mutex_t mutex1;
static gtthread_mutex_t mutex2;

void *thr1(void *in) {
	int i;
	printf("thr1 waiting for mutex1\n");
	fflush(stdout);
	gtthread_mutex_lock(&mutex1);
	printf("thr1 locked mutex1\n");
	fflush(stdout);
	for(i = 0; i < 1000000; i++) {
	}
	gtthread_mutex_unlock(&mutex1);
	printf("thr1 unlocked mutex1\n");
	fflush(stdout);
	printf("thr1 returning\n");
	fflush(stdout);
  return "thr1";
}

void *thr2(void *in) {
	int i;
	printf("thr2 waiting for mutex2\n");
	fflush(stdout);
	gtthread_mutex_lock(&mutex2);
	printf("thr2 locked mutex2\n");
	fflush(stdout);
	for(i = 0; i < 2000000; i++) {
	}
	printf("thr1 unlocked mutex2\n");
	fflush(stdout);
	gtthread_mutex_unlock(&mutex2);
	printf("thr2 returning\n");
	fflush(stdout);
  return "thr2";
}

void *thr3(void *in) {
	int i;
	for(i = 0; i < 3000000; i++) {
	}
	printf("thr3 returning\n");
	fflush(stdout);
  return "thr3";
}

int main() {
  int i;
  gtthread_t t1;
  gtthread_t t2;
  gtthread_t t3;
  void *retval1 = NULL;
  void *retval2 = NULL;
  void *retval3 = NULL;

  gtthread_mutex_init(&mutex1);
  gtthread_mutex_init(&mutex2);

  gtthread_init(5000);

  // acquire mutexes
  printf("main waiting for mutex1\n");
  fflush(stdout);
  gtthread_mutex_lock(&mutex1);
  printf("main locked mutex1\n");
  fflush(stdout);
  printf("main waiting for mutex2\n");
  fflush(stdout);
  gtthread_mutex_lock(&mutex2);
  printf("main locked mutex2\n");
  fflush(stdout);

  printf("main creating threads\n");
  fflush(stdout);
  gtthread_create( &t1, &thr1, NULL);
  gtthread_create( &t2, &thr2, NULL);
  gtthread_create( &t3, &thr3, NULL);

  printf("delaying main thread\n");
  fflush(stdout);
  for (i = 0; i < 100000000; i++) {
  }
  printf("main unlock mutex1\n");
  fflush(stdout);
  gtthread_mutex_unlock(&mutex1);

  printf("delaying main thread\n");
  fflush(stdout);
  for (i = 0; i < 100000000; i++) {
  }
  printf("main unlock mutex2\n");
  fflush(stdout);
  gtthread_mutex_unlock(&mutex2);

  printf("waiting for all threads to finish\n");
  fflush(stdout);
  gtthread_join(t1, &retval1);
  gtthread_join(t2, NULL);
  gtthread_join(t3, &retval3);

  printf("retval1: %s\n", (char*)retval1);
  printf("retval2: %s\n", (char*)retval2);
  printf("retval3: %s\n", (char*)retval3);

  return EXIT_SUCCESS;
}
