#include "seqsrchst.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int
eqfunc(seqsrchst_key a, seqsrchst_key b)
{
  int x, y;

  x = *(int *)a;
  y = *(int *)b;

  if (x == y) {
    return 1;
  } else {
    return 0;
  }
}

int main()
{
  seqsrchst_t seqsrch;
  int size, isempty, contains;
  seqsrchst_key keyp;
  seqsrchst_value valp;
#define MAX_VAL_SZ	10 /* 10 bytes string */

  printf("------------------------------\n");
  printf("init\n");
  seqsrchst_init(&seqsrch, eqfunc);

  isempty = seqsrchst_isempty(&seqsrch);
  printf("isempty=%d\n", isempty);

  size = seqsrchst_size(&seqsrch);
  printf("size=%d\n", size);

  printf("------------------------------\n");
  printf("put 1:ONE\n");
  keyp = (seqsrchst_key)calloc(1, sizeof(int));
  valp = (seqsrchst_key)calloc(1, MAX_VAL_SZ);
  *(int *)keyp = 1;
  strncpy(valp, "ONE", MAX_VAL_SZ);
  seqsrchst_put(&seqsrch, keyp, valp);

  printf("put 2:TWO\n");
  keyp = (seqsrchst_key)calloc(1, sizeof(int));
  valp = (seqsrchst_key)calloc(1, MAX_VAL_SZ);
  *(int *)keyp = 2;
  strncpy(valp, "TWO", MAX_VAL_SZ);
  seqsrchst_put(&seqsrch, keyp, valp);

  printf("put 3:THREE\n");
  keyp = (seqsrchst_key)calloc(1, sizeof(int));
  valp = (seqsrchst_key)calloc(1, MAX_VAL_SZ);
  *(int *)keyp = 3;
  strncpy(valp, "THREE", MAX_VAL_SZ);
  seqsrchst_put(&seqsrch, keyp, valp);

  printf("------------------------------\n");
  size = seqsrchst_size(&seqsrch);
  printf("size=%d\n", size);

  isempty = seqsrchst_isempty(&seqsrch);
  printf("isempty=%d\n", isempty);

  contains = seqsrchst_contains(&seqsrch, keyp);
  printf("contains keyp(%d) =%d\n", *(int *)keyp, contains);

  valp = seqsrchst_get(&seqsrch, keyp);
  printf("get: keyp(%d) val=%s\n", *(int *)keyp, (char *)valp);

  printf("------------------------------\n");
  valp = seqsrchst_delete(&seqsrch, keyp);
  printf("delete: keyp(%d) val=%p\n", *(int *)keyp, valp);

  size = seqsrchst_size(&seqsrch);
  printf("size=%d\n", size);

  contains = seqsrchst_contains(&seqsrch, keyp);
  printf("contains keyp(%d) =%d\n", *(int *)keyp, contains);

  valp = seqsrchst_get(&seqsrch, keyp);
  printf("get: keyp(%d) val=%s\n", *(int *)keyp, (char *)valp);

  valp = seqsrchst_delete(&seqsrch, keyp);
  printf("delete: keyp(%d) val=%p\n", *(int *)keyp, valp);

  seqsrchst_destroy(&seqsrch);
  printf("------------------------------\n");

  return 0;
}
