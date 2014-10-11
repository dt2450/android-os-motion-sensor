#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/syscall.h>
#include <errno.h>

#define N_PROC_NUM		 50
#define __NR_set_acceleration	378
#define __NR_accevt_create	379
#define __NR_accevt_wait	380
#define __NR_accevt_signal	381
#define __NR_accevt_destroy	382

int num_processes = N_PROC_NUM;

#if 0
struct dev_acceleration {
	int x; /* acceleration along X-axis */
	int y; /* acceleration along Y-axis */
	int z; /* acceleration along Z-axis */
};
#endif

struct acc_motion {

	unsigned int dlt_x; /* +/- around X-axis */
	unsigned int dlt_y; /* +/- around Y-axis */
	unsigned int dlt_z; /* +/- around Z-axis */
	unsigned int frq;   /* Number of samples that satisfies:
			       sum_each_sample(dlt_x + dlt_y + dlt_z) > NOISE */
};

int validate_input(int argc, char **argv)
{
	if (argc == 1) {
		printf("Creating default %d number of processes\n",
				num_processes);
		printf("You can also specify the processes as below:\n");
		printf("usage: cmd [<num_processes>]\n");
	} else if (argc != 2) {
		printf("Incorrect no. of args\n");
		printf("usage: cmd [<num_processes>]\n");
		return -1;
	} else {
		num_processes = atoi(argv[1]);
		if (num_processes < 0) {
			printf("Incorrect no. of args\n");
			printf("usage: cmd [<num_processes>]\n");
			return -1;
		}
		printf("Creating %d number of processes\n", num_processes);
	}
	return 0;
}

int main(int argc, char **argv)
{
	int returnValue = validate_input(argc, argv);
	int i = 0;
	if (returnValue == -1)
		return returnValue;

	for (i = 0; i < num_processes; i++) {
		int pid = fork();
		if (pid < 0) {
			printf("Sorry encountered an error while forking.");
			printf(" Aborting from here\n");
			exit(-1);
		} else if (pid == 0) {
			/* child process */
			int event_id = 0;
			int motion_type = i%7;
			struct acc_motion *motion = (struct acc_motion *) malloc
				(sizeof(struct acc_motion));
			//for debugging
			printf("I am child number: %d pid = %d\n", i+1,
					getpid());
			switch(motion_type) {
			case 0:
				motion->dlt_x = (i+1)*100;
				motion->dlt_y = 0;
				motion->dlt_z = 0;
				break;
			case 1:
				motion->dlt_x = 0;
				motion->dlt_y = (i+1)*100;
				motion->dlt_z = 0;
				break;
			case 2:
				motion->dlt_x = 0;
				motion->dlt_y = 0;
				motion->dlt_z = (i+1)*100;
				break;
			case 3:
				motion->dlt_x = (i+1)*100;
				motion->dlt_y = (i+1)*200;
				motion->dlt_z = 0;
				break;
			case 4:
				motion->dlt_x = 0;
				motion->dlt_y = (i+1)*100;
				motion->dlt_z = (i+1)*200;
				break;
			case 5:
				motion->dlt_x = (i+1)*100;
				motion->dlt_y = 0;
				motion->dlt_z = (i+1)*200;
				break;
			case 6:
				motion->dlt_x = (i+1)*100;
				motion->dlt_y = (i+1)*200;
				motion->dlt_z = (i+1)*300;
				break;
			default:
				printf("I don't know how child %d came here\n",
						i+1);
				exit(-1);
			}
			/* frequency is taken randomly between 1 to 20 */
			srand(time(NULL));
			motion->frq= (rand()%20)+1;
			/* create the event with the motion*/
			event_id = syscall(__NR_accevt_create, motion);
			//for debugging
			printf("event id is %d\n", event_id);
			//for debugging
			printf("Syscall returned: %ld.\n",
					syscall(__NR_accevt_wait, event_id));
			printf("%d Detected a x:%d y:%d z:%d shake\n",
					getpid(),
					motion->dlt_x,
					motion->dlt_y,
					motion->dlt_z);
			exit(0);
		} else {
			/* parent process */
		}
	}
	return 0;


	/*

	event_id = syscall(__NR_accevt_create, motion);
	printf("event id is %d\n", event_id);
	printf("Syscall returned: %ld.\n", syscall(__NR_accevt_wait, event_id));

	motion->dlt_x = 10;
	motion->dlt_y = 0;
	motion->dlt_z = 10;
	motion->frq = 2;
	event_id = syscall(__NR_accevt_create, motion);
	printf("event id is %d\n", event_id);
	printf("Syscall returned: %ld.\n", syscall(__NR_accevt_wait, event_id));

	motion->dlt_x = 0;
	motion->dlt_y = 5;
	motion->dlt_z = 5;
	motion->frq = 3;
	event_id = syscall(__NR_accevt_create, motion);
	printf("event id is %d\n", event_id);
	printf("Syscall returned: %ld.\n", syscall(__NR_accevt_wait, event_id));

	motion->dlt_x = 15;
	motion->dlt_y = 15;
	motion->dlt_z = 15;
	motion->frq = 4;
	event_id = syscall(__NR_accevt_create, motion);
	printf("event id is %d\n", event_id);
	printf("Syscall returned: %ld.\n", syscall(__NR_accevt_wait, event_id));

	motion->dlt_x = 25;
	motion->dlt_y = 25;
	motion->dlt_z = 0;
	motion->frq = 2;
	event_id = syscall(__NR_accevt_create, motion);
	printf("event id is %d\n", event_id);
	printf("Syscall returned: %ld.\n", syscall(__NR_accevt_wait, event_id));
	*/
	/*printf("Syscall returned: %d.\n", syscall(__NR_accevt_wait, 0));	
	  printf("Syscall returned: %d.\n", syscall(__NR_accevt_signal, motion));	
	  printf("Syscall returned: %d.\n", syscall(__NR_accevt_destroy, 0));	
	 */
	return 0;
}
