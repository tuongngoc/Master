#include "indexminpq.h"
#include <stdio.h>

int cmpfunc(indexminpq_key a, indexminpq_key b) 
{
	if (*(int *)a == *(int *)b) {
		return 0;
	} else {
		if (*(int *)a < *(int *)b) {
			return -1;
		} else {
			return 1;
		}
	}
}


int
main()
{
	indexminpq_t impq;
	int max_pqe = 5;
	int ret;
	static int val05 = 5;
	static int val10 = 10;
	static int val20 = 20;
	static int val30 = 30;
	static int val40 = 40;
	static int val100 = 100;
	static int val200 = 200;
	indexminpq_key valp;
	

	indexminpq_init(&impq, max_pqe, cmpfunc);

	ret = indexminpq_isempty(&impq);
	printf("isempty = %d\n", ret);

	indexminpq_insert(&impq, 1, (indexminpq_key)&val40);
	printf("insert 1 = %d\n", *(int *)indexminpq_keyof(&impq, 1));

	indexminpq_changekey(&impq, 1, &val10);
	printf("Amy changekey key=1 is %d now\n", val10);
	
	ret = indexminpq_contains(&impq, 1);
	printf("AMY contains 1 = %d\n", ret);

	indexminpq_insert(&impq, 2, (indexminpq_key)&val30);
	printf("insert 2 = %d\n", *(int *)indexminpq_keyof(&impq, 2));

	indexminpq_insert(&impq, 3, (indexminpq_key)&val20);
	printf("insert 3 = %d\n", *(int *)indexminpq_keyof(&impq, 3));

	indexminpq_insert(&impq, 4, (indexminpq_key)&val10);
	printf("insert 4 = %d\n", *(int *)indexminpq_keyof(&impq, 4));

	ret = indexminpq_isempty(&impq);
	printf("isempty = %d\n", ret);

	ret = indexminpq_contains(&impq, 1);
	printf("contains 1 = %d\n", ret);

	ret = indexminpq_contains(&impq, 4);
	printf("contains 4 = %d\n", ret);

	ret = indexminpq_size(&impq);
	printf("size = %d\n", ret);

	printf("delete 4\n");
	indexminpq_delete(&impq, 4);

	ret = indexminpq_contains(&impq, 4);
	printf("contains 4 = %d\n", ret);

	ret = indexminpq_size(&impq);
	printf("size = %d\n", ret);

	/* returns index for the minimum value in PQ */
	ret = indexminpq_minindex(&impq);
	printf("minindex = %d\n", ret);

	ret = indexminpq_minindex(&impq);
	printf("minindex = %d\n", ret);

	/* returns min value in PQ */
	valp = indexminpq_minkey(&impq);
	printf("valp for minkey = %d\n", *((int* )valp));

	valp = indexminpq_minkey(&impq);
	printf("valp for minkey = %d\n", *((int* )valp));

	/* change key for i=3 to 5 now */
	indexminpq_changekey(&impq, 3, &val100);
	printf("changekey for key=3 is 100 now\n");

	/* returns index for the minimum value in PQ */
	ret = indexminpq_minindex(&impq);
	printf("minindex = %d\n", ret);

	/* returns min value in PQ */
	valp = indexminpq_minkey(&impq);
	printf("valp for minkey = %d\n", *((int* )valp));

	ret = indexminpq_isempty(&impq);
	if (ret == 0) {
		/* not empty: delete min key */
		ret = indexminpq_delmin(&impq);
		printf("delmin = %d\n", ret);
	} else {
		printf("isempty = %d, skipping indexminpq_delmin\n", ret);
	}
	ret = indexminpq_size(&impq);
	printf("size = %d\n", ret);

	/* get value for key=3 */
	valp = indexminpq_keyof(&impq, 3);
	printf("valp for key=3 is %d\n", *((int* )valp));

	/* change key for i=3 to 100 */
	indexminpq_changekey(&impq, 3, &val100);
	printf("changekey for key=3 is 100 now\n");

	/* get value for key3 */
	valp = indexminpq_keyof(&impq, 3);
	printf("valp for key=3 is %d\n", *((int* )valp));

	/* +/swim-up key value for key=3 by increasing val from 100 to 10 */
	indexminpq_increasekey(&impq, 3, &val10);
	printf("increasekey/swim-up for key=3 is %d now\n", val10);

	/* get value for key=3 */
	valp = indexminpq_keyof(&impq, 3);
	printf("valp for key=2 is %d\n", *((int* )valp));

	/* -/sink-down key value for key=3 by decreasing val from 10 to 20 */
	indexminpq_decreasekey(&impq, 3, &val20);
	printf("decrease/sink-down for key=3 is %d now\n", val20);

	/* get value for key=3 */
	valp = indexminpq_keyof(&impq, 3);
	printf("valp for key=3 is %d\n", *((int* )valp));

	indexminpq_destroy(&impq);

	return 0;
}

