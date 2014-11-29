//#include "seqsrchst.h"
#include "steque.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define TEST_STRING1 "hello_1"
#define TEST_STRING2 "hello_2"
#define TEST_STRING3 "BINGO"

#define OFFSET1 0
#define OFFSET2 1000
#define OFFSET3 2000

#define MAX_VAL_SZ	10 /* 10 bytes string */

typedef struct mod_data_t{
  int offset;
  int size;
  void *undo;
} mod_data_t;

static steque_t *stack_mods;

int main()
{
	int i = 0;
	int size;
	int isempty;
	int max_entries = 5;
	mod_data_t *pdata;
	mod_data_t** mods = (mod_data_t**)calloc(max_entries, sizeof(mod_data_t *));
	if (mods == NULL) {
		printf( "\n**can't allocate memmory for mods\n");
		exit(EXIT_FAILURE);
	} 
	
	/* build the stack mods table*/
	stack_mods = malloc (sizeof(steque_t));
	if (stack_mods == NULL) {
		printf( "\n can't allocate memmory for stack_of_ids\n");
		exit(EXIT_FAILURE);
	} 	
	
	printf("------------------------------\n");
	printf("init\n");
	steque_init(stack_mods);
	printf("------------------------------\n");
	printf("add Mod[0]:[offset:0,size:100,data:<hello1> \t\n");	
	mods[0] = malloc(sizeof(mod_data_t));
	mods[0]->offset = OFFSET1;
	mods[0]->size = 100;
	mods[0]->undo = malloc(mods[0]->size);
	strncpy(mods[0]->undo,TEST_STRING1, 100);
	steque_enqueue(stack_mods, mods[0]);
	
	printf("add Mod[1]:[offset:1000,size:100,data:<hello2> \t\n");
	mods[1] = malloc(sizeof(mod_data_t));
	mods[1]->offset = OFFSET2;
	mods[1]->size = 100;
	mods[1]->undo = malloc(mods[1]->size);
	strncpy(mods[1]->undo,TEST_STRING2, 100);
	steque_enqueue(stack_mods, mods[1]);

	printf("add Mod[2]:[offset:1000,size:100,data:<BINGO> \t\n");
	mods[2] = malloc(sizeof(mod_data_t));
	mods[2]->offset = OFFSET3;
	mods[2]->size = 100;
	mods[2]->undo = malloc(mods[1]->size);
	strncpy(mods[2]->undo,TEST_STRING3, 100);
	steque_enqueue(stack_mods, mods[2]);

	printf("------------------------------\n");
	size = steque_size(stack_mods);
	printf("size=%d\n", size);

	isempty = steque_isempty(stack_mods);
	printf("isempty=%d\n", isempty);

	printf("------------------------------\n");
	/*Returns the element at the "front" of the steque without removing it*/
	pdata = (mod_data_t *)steque_front(stack_mods);
	printf("steque_front- offset:(%d) size=%d, undo: %s\n", 
		pdata->offset, pdata->size, (char*)pdata->undo);

	size = steque_size(stack_mods);
	printf("*QUE-size=%d\n", size);
	printf("------------------------------\n");
	for (i = 0; i < size ; ++i) {
		pdata = (mod_data_t *) steque_pop(stack_mods);	
		printf("steque i= %d offset:(%d) size=%d, undo: %s\n", 
				i, pdata->offset, pdata->size, (char*)pdata->undo);
	}
	printf("------------------------------\n");

  printf ("\n call steque_destroy()..\n");
	steque_destroy(stack_mods);
	free(mods);
	printf("------------------------------\n");

	return 0;
}

