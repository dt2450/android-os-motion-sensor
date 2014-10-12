#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <errno.h>


#define __NR_set_acceleration	378
#define __NR_accevt_create	379
#define __NR_accevt_wait	380
#define __NR_accevt_signal	381
#define __NR_accevt_destroy	382

struct dev_acceleration {
	int x; /* acceleration along X-axis */
	int y; /* acceleration along Y-axis */
	int z; /* acceleration along Z-axis */
};

struct acc_motion {

     unsigned int dlt_x; /* +/- around X-axis */
     unsigned int dlt_y; /* +/- around Y-axis */
     unsigned int dlt_z; /* +/- around Z-axis */
     unsigned int frq;   /* Number of samples that satisfies:
                          sum_each_sample(dlt_x + dlt_y + dlt_z) > NOISE */
};

int events[100];
int idx;

int validate_input(int argc, char **argv)
{
	return 0;
}

void test_1_create_events(void)
{
	idx = 0;
	int event_id = 0;
	struct dev_acceleration *acc = (struct dev_acceleration *) malloc
		(sizeof(struct dev_acceleration));
	/*printf("Syscall returned: %ld.\n", syscall(__NR_set_acceleration, acc));*/

	struct acc_motion *motion = (struct acc_motion *) malloc
		(sizeof(struct acc_motion));
	motion->dlt_x = 5;
	motion->dlt_y = 5;
	motion->dlt_z = 0;
	motion->frq = 1;
	events[idx++] = syscall(__NR_accevt_create, motion);
	printf("event id is %d\n", events[idx-1]);
	//printf("Syscall returned: %ld.\n", syscall(__NR_accevt_wait, event_id));

	motion->dlt_x = 10;
	motion->dlt_y = 0;
	motion->dlt_z = 10;
	motion->frq = 2;
	events[idx++] = syscall(__NR_accevt_create, motion);
	printf("event id is %d\n", events[idx-1]);
	//printf("Syscall returned: %ld.\n", syscall(__NR_accevt_wait, event_id));

	motion->dlt_x = 0;
	motion->dlt_y = 5;
	motion->dlt_z = 5;
	motion->frq = 3;
	events[idx++] = syscall(__NR_accevt_create, motion);
	printf("event id is %d\n", events[idx-1]);
	//printf("Syscall returned: %ld.\n", syscall(__NR_accevt_wait, event_id));

	motion->dlt_x = 15;
	motion->dlt_y = 15;
	motion->dlt_z = 15;
	motion->frq = 4;
	events[idx++] = syscall(__NR_accevt_create, motion);
	printf("event id is %d\n", events[idx-1]);
	//printf("Syscall returned: %ld.\n", syscall(__NR_accevt_wait, event_id));

	motion->dlt_x = 25;
	motion->dlt_y = 25;
	motion->dlt_z = 0;
	motion->frq = 2;
	events[idx++] = syscall(__NR_accevt_create, motion);
	printf("event id is %d\n", events[idx-1]);
	//printf("Syscall returned: %ld.\n", syscall(__NR_accevt_wait, event_id));

}

void test_2_remove_events(void)
{
	int i, ret;
	struct acc_motion *m = NULL;

	for (i=0; i<idx; i++) {
		ret = syscall(__NR_accevt_destroy, events[i]);
		printf("return val is: %d\n", ret);
	}
}

int main(int argc, char **argv)
{
	int returnValue = validate_input(argc, argv);
	if (returnValue == -1)
		return returnValue;

	test_1_create_events();
	test_2_remove_events();
	
	/*printf("Syscall returned: %d.\n", syscall(__NR_accevt_wait, 0));	
	printf("Syscall returned: %d.\n", syscall(__NR_accevt_signal, motion));	
	printf("Syscall returned: %d.\n", syscall(__NR_accevt_destroy, 0));	
	*/
	return 0;
}
