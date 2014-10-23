#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include <math.h>
#include "gtmp.h"

/*Enale debug*/
/*#define ENABLE_DEBUG*/

typedef int bool;
enum {false,true};

/* MCS tree node data structure*/
typedef struct _node_t {
  int id;
  bool parentsense;
  bool *parentpointer;
  /* childpointers : array [0..1]*/
  bool *childpointers[2];
  /*havechild : array [0..3]*/
  bool havechild[4];
  /*childnotready : array [0..3] */
  bool childnotready[4];
  bool dummy;
} node_t;

/* Global nodes*/
static node_t* Gtree_nodes;

/*
 *get node.
 *To return a node data
*/
node_t* get_node(int i);
#ifdef ENABLE_DEBUG
void gtmp_print_node_data(node_t* node);
#endif

/*
From the MCS Paper: A scalable, distributed tree-based barrier with only local spinning.

    type treenode = record
        parentsense : Boolean
	parentpointer : ^Boolean
	childpointers : array [0..1] of ^Boolean
	havechild : array [0..3] of Boolean
	childnotready : array [0..3] of Boolean
	dummy : Boolean //pseudo-data

    shared nodes : array [0..P-1] of treenode
        // nodes[vpid] is allocated in shared memory
        // locally accessible to processor vpid
    processor private vpid : integer // a unique virtual processor index
    processor private sense : Boolean

    // on processor i, sense is initially true
    // in nodes[i]:
    //    havechild[j] = true if 4 * i + j + 1 < P; otherwise false
    //    parentpointer = &nodes[floor((i-1)/4].childnotready[(i-1) mod 4],
    //        or dummy if i = 0
    //    childpointers[0] = &nodes[2*i+1].parentsense, or &dummy if 2*i+1 >= P
    //    childpointers[1] = &nodes[2*i+2].parentsense, or &dummy if 2*i+2 >= P
    //    initially childnotready = havechild and parentsense = false

    procedure tree_barrier
        with nodes[vpid] do
	    repeat until childnotready = {false, false, false, false}
	    childnotready := havechild //prepare for next barrier
	    parentpointer^ := false //let parent know I'm ready
	    // if not root, wait until my parent signals wakeup
	    if vpid != 0
	        repeat until parentsense = sense
	    // signal children in wakeup tree
	    childpointers[0]^ := sense
	    childpointers[1]^ := sense
	    sense := not sense
*/

void gtmp_init(int num_threads)
{
  node_t* current_node = NULL;
  int i = 0;
  int j = 0;

  Gtree_nodes = (node_t*) malloc(num_threads * sizeof(node_t));
  if (Gtree_nodes == NULL) {
    fprintf(stderr, "Failed to allocate memory.\n");
    exit(-1);
  }

  for (i = 0; i < num_threads; i++) {

    /* point to the current node*/
    current_node = get_node(i);
    current_node->id = i;

    current_node->parentsense = false;

    for (j = 0; j < 4; j++) {
      if (4 * i + j + 1 < num_threads) {
        current_node->havechild[j] = true;
      } else {
        current_node->havechild[j] = false;
      }
      current_node->childnotready[j] = current_node->havechild[j];
    }
  }

  for (i = 0; i < num_threads; i++) {
    current_node = get_node(i);
    if (i == 0) {
      current_node->parentpointer = &(current_node->dummy);
    } else {
      current_node->parentpointer = &(get_node(
	   (int)((i - 1) / 4))->childnotready[(i - 1) % 4]);
    }

    if (2 * i + 1 < num_threads) {
      current_node->childpointers[0] =
        &(get_node(2 * i + 1)->parentsense);
    } else {
      current_node->childpointers[0] = &(current_node->dummy);
    }

    if (2 * i + 2 < num_threads) {
      current_node->childpointers[1] =
        &(get_node(2 * i + 2))->parentsense;
    } else {
      current_node->childpointers[1] = &(current_node->dummy);
    }

#ifdef ENABLE_DEBUG
    gtmp_print_node_data(current_node);
#endif

  }
}

void gtmp_barrier()
{
  bool local_sense;
  int tid;
  int sum;
  int i;

  tid = omp_get_thread_num();

  node_t* current_node = get_node(tid);

#ifdef ENABLE_DEBUG
  gtmp_print_node_data(current_node);
#endif

  local_sense = !(current_node->parentsense);

  sum = 4;
  //when all children are ready, proceed past while loop
  while (sum) {
    sum = 0;
    for (i = 0; i < 4; i++) {
      sum += current_node->childnotready[i];
    }
  }
  //reset for next barrier
  for (i = 0; i < 4; i++) {
    current_node->childnotready[i] = current_node->havechild[i];
  }
  //tell parent that I'm ready
  *(current_node->parentpointer) = false;

  //wait for the root thread
  while (tid != 0) {
    if (current_node->parentsense == local_sense) {
      break;
    }
  }

  for (i = 0; i < 2; i++) {
    *(current_node->childpointers[i]) = local_sense;
  }

  if (tid == 0) {
    current_node->parentsense = local_sense;
  }

}

void gtmp_finalize() {
  if (Gtree_nodes != NULL) {
    free(Gtree_nodes);
  }
}


node_t* get_node(int i)
{
  return &Gtree_nodes[i];
}

#ifdef ENABLE_DEBUG
void gtmp_print_node_data(node_t* node) {
  int j;

  printf("-----------------Node Info-----------------\n");

  printf("id = %d\n", node->id);

  printf("parentsense = %d\n", node->parentsense);

  printf("&parentsense = %p\n", &(node->parentsense));

  printf("parentpointer = %p\n", (void*) node->parentpointer);

  printf("*parentpointer = %d\n", *(node->parentpointer));

  printf("childpointers = [%p,%p] \n", node->childpointers[0], node->childpointers[1]);

  printf("*childpointers = [%d,%d] \n", *(node->childpointers[0]),
         *(node->childpointers[1]));

  printf("havechild = [");
  for (j = 0; j < 4; j++) {
    printf("%d ", node->havechild[j]);
  }
  printf("]\n");

  printf("childnotready = [");
  for (j = 0; j < 4; j++) {
    printf("%d ", node->childnotready[j]);
  }
  printf("]\n");

  printf("childnotready = [");
  for (j = 0; j < 4; j++) {
    printf("%p ", &(node->childnotready[j]));
  }
  printf("]\n");

  printf("dummy = %d\n", node->dummy);
  printf("---------End of Node Info-----------\n");

}
#endif /*#ifdef ENABLE_DEBUG*/
