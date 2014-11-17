#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "steque.h"

/* 
* Test steque program to better understand 
*/
int main()
{
  static steque_t *stack_ids;
	int i = 0;
	int size;
	int max_entries = 5;
	int id ;
	
	/* build the stack ids table*/
		stack_ids = malloc (sizeof(steque_t));
		if (stack_ids == NULL) {
			printf( "\n can't allocate memmory for stack_of_ids\n");
			exit(EXIT_FAILURE);
		}
		steque_init(stack_ids);
		for (i = 0 ; i < (max_entries -1) ; i ++) {
			/* Adds an element to the "front" of the steque */
			//steque_push(stack_ids,i);	
			/* Adds an element to the "back" of the steque */
			steque_enqueue(stack_ids,i);		
		}
		size = steque_size(stack_ids);
		printf("\n--steque size %d\n", size);
		id = (int)steque_pop(stack_ids);
		printf("\n==>id1 = %d\n", id);
		id = (int)steque_pop(stack_ids);
		printf("\n==>id2 = %d\n", id);
		id = (int)steque_pop(stack_ids);
		printf("\n==>id3 = %d\n", id);
		id = (int)steque_pop(stack_ids);
		printf("\n==>id4 = %d\n", id);
		id = (int)steque_pop(stack_ids);
		printf("\n==>id5 = %d\n", id);
	
	return 0;

}