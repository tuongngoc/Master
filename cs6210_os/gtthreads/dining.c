#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>


typedef enum  {left , right } direction_t;

pthread_mutex_t chopstick_mutex[5];


int phil_to_chopstick(int phil_id, direction_t d){
  return (phil_id + d) % 5;      
}

int chopstick_to_phil(int stick_id, direction_t d){
  return (stick_id + 5 - d) % 5;      
}

void pickup_one_chopstick(int stick_id, int phil_id){
  /*
  Use the line

  printf("Philosopher %d picks up chopstick %d \n", phil_id, stick_id);

  to have the philosopher pick-up a chopstick. Debugging will be easier
  if you remember to flush stdout.

  fflush(stdout)
  */
  /*Your code here*/
  printf("Philosopher %d picks up chopstick %d \n", phil_id, stick_id);
  fflush(stdout);

}
void putdown_one_chopstick(int stick_id, int phil_id){
  /*
  Use the line

  printf("Philosopher %d puts down chopstick %d \n", phil_id, stick_id);

  to have the philosopher pick-up a chopstick. Debugging will be easier
  if you remember to flush stdout.

  fflush(stdout)
  */
  /*Your code here*/
  printf("Philosopher %d puts down chopstick %d \n", phil_id, stick_id);
  pthread_mutex_unlock(&chopstick_mutex[stick_id]);	/* unlock mutex */  
  fflush(stdout);
}

/*=====================================================
* Function:
* pickup_chopsticks:  input philoshoper id
*
* Try to take both chopstick. If it's not available, then don't take it
* Logic: Try to take the 1st chopstick on the right, and then try to take the left.
*           if left is not available, then just release the right chopstick.
=====================================================*/
void pickup_chopsticks(int phil_id){
  /*Use     
  pickup_chopstick(phil_to_chopstick(phil_id, right), phil_id);

  and

  pickup_chopstick(phil_to_chopstick(phil_id, left), phil_id);

  to pickup the right and left chopsticks.  Beware of deadlock!
  */
  for (;;) {
    if (pthread_mutex_trylock(&chopstick_mutex[phil_to_chopstick(phil_id, right)]) == 0) {
      if (pthread_mutex_trylock(&chopstick_mutex[phil_to_chopstick(phil_id, left)]) == 0) {
        pickup_one_chopstick(phil_to_chopstick(phil_id, right), phil_id);
        pickup_one_chopstick(phil_to_chopstick(phil_id, left), phil_id);
        break;
      }  
      pthread_mutex_unlock(&chopstick_mutex[phil_to_chopstick(phil_id, right)]);
    }
    //printf("Philosopher %d wait\n", phil_id);
    sched_yield();
  } /*end forever*/

}

/*=====================================================
* Function:
* putdown_chopsticks:  input philoshoper id
*
* Release both chopstick and yield 
=====================================================*/

void putdown_chopsticks(int phil_id){
  /*Use putdown_chopstick to put down the chopsticks*/    
  /*Your code here*/
  putdown_one_chopstick(phil_to_chopstick(phil_id, left), phil_id);
  putdown_one_chopstick(phil_to_chopstick(phil_id, right), phil_id);
  sched_yield();
}



