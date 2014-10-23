#ifndef GTTHREAD_H
#define GTTHREAD_H

#include "steque.h"
#include <ucontext.h>

/* Define gtthread_t and gtthread_mutex_t types here */
#define gtthread_id  int
#define STACK_SIZE 65536
#define MAX_THREAD 100

typedef int bool;
enum { false, true };

typedef struct gtthread{
	/* thread states*/
	bool active;
	bool finished;
	bool cancel;
	gtthread_id thread_id;
	gtthread_id joined_tid;
	ucontext_t thread_context;
	/* The return value of the thread. */
	void *returnval;
}gtthread_t;


typedef struct gtthread_mutex_t{
	int lock;
	gtthread_id owner; /*Id of task locking mutex */
	steque_t mutex_queue;
}gtthread_mutex_t;

/*extern global data*/
extern gtthread_t* current_thread;

void gtthread_init(long period);
int  gtthread_create(gtthread_t *thread,
                     void *(*start_routine)(void *),
                     void *arg);
int  gtthread_join(gtthread_t thread, void **status);
void gtthread_exit(void *retval);
void gtthread_yield(void);
int  gtthread_equal(gtthread_t t1, gtthread_t t2);
int  gtthread_cancel(gtthread_t thread);
gtthread_t gtthread_self(void);


int  gtthread_mutex_init(gtthread_mutex_t *mutex);
int  gtthread_mutex_lock(gtthread_mutex_t *mutex);
int  gtthread_mutex_unlock(gtthread_mutex_t *mutex);
int  gtthread_mutex_destroy(gtthread_mutex_t *mutex);


#endif
