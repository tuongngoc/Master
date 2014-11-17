#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "indexrndq.h"

int main()
{
  indexrndq_t xrndq;
  int max_dq = 7;
  int ret = 0;
  static int val05 = 5;
  int i = 0;
  
  indexrndq_init(&xrndq, max_dq);
  
  ret = indexrndq_isempty(&xrndq);
  printf("isempty = %d\n", ret);
  
  ret = indexrndq_contains(&xrndq, 2);
  printf("contain_2 = %d\n", ret);
  
  for (i = 0; i < max_dq ; i++) {
    indexrndq_enqueue(&xrndq, i);
  }
  ret = indexrndq_contains(&xrndq, 2);
  printf("-->contain_2 = %d\n", ret);
  
  ret = indexrndq_contains(&xrndq, 0);
  printf("contain_0 = %d\n", ret);
  ret = indexrndq_dequeue(&xrndq);
  printf("-dequeue = %d\n", ret);
  ret = indexrndq_contains(&xrndq, 0);
  printf("-> after dequeue++contain_0 = %d\n", ret);
  

  return 0;
}