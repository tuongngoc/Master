/**********************************************************************
gtthread_sched.c.  

This file contains the implementation of the scheduling subset of the
gtthreads library.  A simple round-robin queue should be used.
Author: Ngoc(Amy) Tran
StudentID: 903094708
 **********************************************************************/
/*
  Include as needed
*/
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include "gtthread.h"

/*
   Students should define global variables and helper functions as
   they see fit.
 */
#define LOG_LEVEL 0
/* Macros for printing messages. */
#define log_msg(level, msg) if ( (level) <= LOG_LEVEL ) { fprintf(stderr, msg); }
#define fatal_error(msg) fprintf(stderr, msg); exit(EXIT_FAILURE);

/* Some global constants. */
gtthread_t main_thread;
gtthread_t* current_thread;
gtthread_t* gtthread_array[MAX_THREAD];

static int thread_count = 0;

/* Queue for the threads schedule */
steque_t ready_queue;
static sigset_t vtalrm;

/*Thread initialize status*/
static bool initialized = false;

static void run_thread(void *(*start_routine)(void *), void* args) {
  void * retval = start_routine(args);
  if (gtthread_equal(*current_thread, main_thread)) {
    int *value = (int *)retval;
    exit(*value);
  } 
  else {
    /* Run the function and gtthread_exit it. */
    gtthread_exit(retval);
  }
  return;
}

/* scheduler handler*/
static void scheduler() 
{
  sigprocmask(SIG_BLOCK, &vtalrm, NULL);

  if (steque_isempty(&ready_queue)) {
    // Current thread finished and no more threads to run.
    if (current_thread->finished || current_thread->cancel) {
      // Swap current thread back to main thread.
      ucontext_t * curContext = &current_thread->thread_context;
      current_thread = &main_thread;
      sigprocmask(SIG_UNBLOCK, &vtalrm, NULL);
      swapcontext(curContext, &main_thread.thread_context);
      return;
    }
    // Keep running the current thread since nothing else is queued.
    sigprocmask(SIG_UNBLOCK, &vtalrm, NULL);
    return;
  }

  gtthread_t* nextThread = steque_pop(&ready_queue);

  // Do not enqueue thread that already terminated.
  if (!current_thread->finished &&
      !current_thread->cancel) {
      steque_enqueue(&ready_queue, current_thread);
  }
  ucontext_t * currentContext = NULL;
  if (current_thread != NULL) {
    currentContext = &current_thread->thread_context;
  } else {
    currentContext = &main_thread.thread_context;
  }
  current_thread = nextThread;
  sigprocmask(SIG_UNBLOCK, &vtalrm, NULL);
  swapcontext(currentContext, &nextThread->thread_context);
}


/*
  The gtthread_init() function does not have a corresponding pthread equivalent.
  It must be called from the main thread before any other GTThreads
  functions are called. It allows the caller to specify the scheduling
  period (quantum in micro second), and may also perform any other
  necessary initialization.  If period is zero, then thread switching should
  occur only on calls to gtthread_yield().

  Recall that the initial thread of the program (i.e. the one running
  main() ) is a thread like any other. It should have a
  gtthread_t that clients can retrieve by calling gtthread_self()
  from the initial thread, and they should be able to specify it as an
  argument to other GTThreads functions. The only difference in the
  initial thread is how it behaves when it executes a return
  instruction. You can find details on this difference in the man page
  for pthread_create.
 */
void gtthread_init(long period){
  if ( !initialized) {
    initialized = true;
    log_msg(3, "Starting gtthread_init()\n");
    /* Initialize the threads queue. */
    steque_init(&ready_queue);
    /* Set up the context for main and create a thread for it. */
    main_thread.thread_id = -100; /* assign my own id*/
    main_thread.joined_tid = -1;
    main_thread.active = false;
    main_thread.finished = false;
    main_thread.cancel= false;  
    current_thread = &main_thread;
    if (getcontext(&main_thread.thread_context) == -1) {
      fatal_error("Error: During getcontext");
    }
    main_thread.thread_context.uc_stack.ss_sp = (char*) malloc(SIGSTKSZ);
    main_thread.thread_context.uc_stack.ss_size = SIGSTKSZ;
    /*
    Setting up the signal mask
    */
    sigemptyset(&vtalrm);
    sigaddset(&vtalrm, SIGVTALRM);
    /*Set up a signal handler*/
    struct sigaction preempt_act;
    memset (&preempt_act, '\0', sizeof(preempt_act));
    preempt_act.sa_handler = &scheduler;
    if (sigaction(SIGVTALRM, &preempt_act, NULL) == -1) {
      printf("**Error(gtthread_init) - preempt signal_handler failed\n");
      exit(1);
    }
    /* Set up the timer intervals. */
    if (period > 0) {
      struct itimerval* ptime_slide = (struct itimerval*) malloc(sizeof(struct itimerval));
      ptime_slide->it_value.tv_sec = 0;
      ptime_slide->it_value.tv_usec = period;
      ptime_slide->it_interval.tv_sec = 0;
      ptime_slide->it_interval.tv_usec = period;
      /*start the timer */
      setitimer(ITIMER_VIRTUAL, ptime_slide, NULL);
    }
  }
  else {
    printf("*(error)* Just initialize only once\n");
    exit(-1);
  }
}


/*
  The gtthread_create() function mirrors the pthread_create() function,
  only default attributes are always assumed.
 */
 int gtthread_create(gtthread_t *thread,
                    void *(*start_routine)(void *),
                    void *arg)
 {
   if( !initialized ) {
     printf("\nCreate : Error: gtthread_init() not called! Exiting.\n");
     exit(-1);
   }	

   log_msg(3, "Starting gtthread_create()\n");
   thread->thread_id = thread_count++;
   thread->joined_tid = -1;
   thread->finished = false;
   thread->cancel = false;
   if (thread->thread_id >= MAX_THREAD) {
     printf("***Error! Can not go over the MAX of supported thread");
     exit(-1);
   }
   /* Make the context. */
   if (getcontext(&thread->thread_context) == -1) {
     fatal_error("**Error**: getcontext()");
   }
   thread->thread_context.uc_stack.ss_sp = malloc(STACK_SIZE);
   thread->thread_context.uc_stack.ss_size = STACK_SIZE;
   thread->thread_context.uc_link = &main_thread.thread_context;
   makecontext(&thread->thread_context, (void (*)())run_thread,
     2, start_routine, arg);

   /* add thread to the queue & global threads array*/			  
   sigprocmask(SIG_BLOCK, &vtalrm, NULL);
   gtthread_array[thread->thread_id] = thread;
   log_msg(3, "Inserting into queue\n");
   /* add elements to the back*/
   steque_enqueue(&ready_queue, thread);
   sigprocmask(SIG_UNBLOCK, &vtalrm, NULL);

   log_msg(3, "Finished gtthread_create()\n");
   return 0;
 }


/*
  The gtthread_join() function is analogous to pthread_join.
  All gtthreads are joinable.
 */
 int gtthread_join(gtthread_t thread, void **status) {
   /* can't join the current thread*/
  if (gtthread_equal(thread, *current_thread)) {
    return -1;
  }
  /*get thread from global array list*/
  gtthread_t* pthread = gtthread_array[thread.thread_id];
  if (current_thread->joined_tid == pthread->thread_id) {
    log_msg(4, "Cannot join.***---DEAD LOCKED---***\n");
    return -1;
  }

  pthread->joined_tid = current_thread->thread_id;

  while (!pthread->finished && !pthread->cancel) {
    gtthread_yield();
  }
  if (status != NULL) {
    *status = pthread->returnval;
  }
  return 0;
}

/*
  The gtthread_exit() function is analogous to pthread_exit.
 */
void gtthread_exit(void* retval) {
  current_thread->returnval = retval;
  current_thread->finished = true;
  raise(SIGVTALRM);
}

/*
  The gtthread_yield() function is analogous to pthread_yield, causing
  the calling thread to relinquish the cpu and place itself at the
  back of the schedule queue.
 */
void gtthread_yield(void){
  /* Reset the timer so that the next thread has a fair time period. */
#if 0  
  stop_timer();
  start_timer();
#endif  
  raise(SIGVTALRM);
  /*scheduler(); */
}

/*
  The gtthread_yield() function is analogous to pthread_equal,
  returning non-zero if the threads are the same and zero otherwise.
 */
int  gtthread_equal(gtthread_t t1, gtthread_t t2){
  /* return 1 if the same*/
  return (t1.thread_id == t2.thread_id);
}

/*
  The gtthread_cancel() function is analogous to pthread_cancel,
  allowing one thread to terminate another asynchronously.
 */
int  gtthread_cancel(gtthread_t thread){
  int status = 0;
  if (thread.thread_id >= MAX_THREAD) {
    status = 1;
    printf("\n **Error** Invalid threadid = %d \n", thread.thread_id );
  }
  else {
    gtthread_array[thread.thread_id]->cancel = true;
  }
  return (status);
}

/*
  Returns calling thread.
 */
gtthread_t gtthread_self(void) {
  return *current_thread;
}

