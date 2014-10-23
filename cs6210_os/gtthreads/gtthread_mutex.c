/**********************************************************************
gtthread_mutex.c.  

This file contains the implementation of the mutex subset of the
gtthreads library.  The locks can be implemented with a simple queue.
 **********************************************************************/

/*
  Include as needed
*/

#include "gtthread.h"

/*
  The gtthread_mutex_init() function is analogous to
  pthread_mutex_init with the default parameters enforced.
  There is no need to create a static initializer analogous to
  PTHREAD_MUTEX_INITIALIZER.
 */
int gtthread_mutex_init(gtthread_mutex_t* mutex){
  if (mutex->lock == 1) return -1;
  mutex->lock = 0;
  mutex->owner = -1;
  /*initial mutex queue*/
  steque_init(&mutex->mutex_queue);
  return 0;
}

/*
  The gtthread_mutex_lock() is analogous to pthread_mutex_lock.
  Returns zero on success.
 */
int gtthread_mutex_lock(gtthread_mutex_t* mutex){
   int *my_id = 0;
   if ((mutex->owner) == current_thread->thread_id) {
     return -1;
    }
	if (mutex->lock == 0) {
    mutex->lock = 1;
    mutex->owner = current_thread->thread_id;
    return 0;
  }
  /* add to the queue*/
  steque_enqueue(&mutex->mutex_queue, &current_thread->thread_id);
  while(mutex->lock != 0 && mutex->owner != current_thread->thread_id) gtthread_yield();
  
  for (;;) {
    my_id  = (int *)steque_front(&mutex->mutex_queue);
    if (*my_id == current_thread->thread_id) {
	  /*remove from the queue if found it */
      steque_pop(&mutex->mutex_queue);
      mutex->lock = 1;
      mutex->owner = current_thread->thread_id;      
      return 0;
    } 
	else {
      gtthread_yield();
    }
  }
  return -1;
}

/*
  The gtthread_mutex_unlock() is analogous to pthread_mutex_unlock.
  Returns zero on success.
 */
 int gtthread_mutex_unlock(gtthread_mutex_t *mutex){
   if (mutex->lock == 1 && mutex->owner == current_thread->thread_id) { 
     mutex->lock = 0;
     mutex->owner = -1;
     return 0;
   }
   return -1;
}

/*
  The gtthread_mutex_destroy() function is analogous to
  pthread_mutex_destroy and frees any resourcs associated with the mutex.
*/
int gtthread_mutex_destroy(gtthread_mutex_t *mutex){
  mutex->lock = -1;
  mutex->owner = -1;
  steque_destroy(&mutex->mutex_queue);
  return 0;
}
