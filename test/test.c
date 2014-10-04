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

int validate_input(int argc, char **argv)
{
	return 0;
}

int main(int argc, char **argv)
{
	int returnValue = validate_input(argc, argv);

	if (returnValue == -1)
		return returnValue;

	struct dev_acceleration *acc = (struct dev_acceleration *) malloc(sizeof(struct dev_acceleration));
	printf("Syscall returned: %d.\n", syscall(__NR_set_acceleration, acc));	

	struct acc_motion *motion = (struct acc_motion *) malloc(sizeof(struct acc_motion));
	printf("Syscall returned: %d.\n", syscall(__NR_accevt_create, motion));	
	printf("Syscall returned: %d.\n", syscall(__NR_accevt_wait, 0));	
	printf("Syscall returned: %d.\n", syscall(__NR_accevt_signal, motion));	
	printf("Syscall returned: %d.\n", syscall(__NR_accevt_destroy, 0));	

	return 0;
}
