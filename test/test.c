#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/syscall.h>
#include <errno.h>

#define DEFAULT_N			30
#define MAX_N				1024
#define DEFAULT_TIMEOUT_IN_SECS		60
#define __NR_set_acceleration		378
#define __NR_accevt_create		379
#define __NR_accevt_wait		380
#define __NR_accevt_signal		381
#define __NR_accevt_destroy		382

static int num_processes = DEFAULT_N;
static int timeout_secs = DEFAULT_TIMEOUT_IN_SECS;

struct acc_motion {

	unsigned int dlt_x; /* +/- around X-axis */
	unsigned int dlt_y; /* +/- around Y-axis */
	unsigned int dlt_z; /* +/- around Z-axis */
	unsigned int frq;   /* Number of samples */
};

int validate_input(int argc, char **argv)
{
	if (argc == 1) {
		printf("Creating default %d number of processes",
				num_processes);
		printf(" for default timeout %d secs\n", timeout_secs);
		printf("You can also specify the values as below:\n");
		printf("usage: cmd <num_processes> <timeout_in_secs>\n");
	} else if (argc != 3) {
		printf("Incorrect no. of args\n");
		printf("usage: cmd <num_processes> <timeout_in_secs>\n");
		goto exit;
	} else {
		num_processes = atoi(argv[1]);
		timeout_secs = atoi(argv[2]);
	}
	if (num_processes < 0 || timeout_secs < 0) {
		printf("Incorrect args. Please verify again\n");
		printf("usage: cmd <num_processes> <timeout_in_secs>\n");
		return -1;
	}
	if (num_processes > MAX_N)
		num_processes = MAX_N;
	printf("Creating %d number of processes\n", num_processes);
	return 0;
exit:
	return -1;
}

int main(int argc, char **argv)
{
	int returnValue = validate_input(argc, argv);
	int event_id_array[num_processes];
	int child_pid_array[num_processes];
	int i = 0;
	int frequency = 0;

	if (returnValue == -1)
		return returnValue;

	memset(&event_id_array, 0, sizeof(event_id_array));
	memset(&child_pid_array, 0, sizeof(child_pid_array));

	for (i = 0; i < num_processes; i++) {
		int pipe_fd[2] = {-1};

		if (pipe(pipe_fd) == -1) {
			printf("Parent process error: %s\n", strerror(errno));
			/*for debugging what if the parent exits and some
			children are waiting in the queue forever?*/
			exit(-1);
		}
		/* frequency is taken randomly between 1 to 20 */
		srand(time(NULL));
		frequency = (rand()%20)+1;

		int pid = fork();

		if (pid < 0) {
			printf("Sorry encountered an error while forking.");
			printf(" Aborting from here\n");
			exit(-1);
		} else if (pid == 0) {
			/* child process */
			/* close the read fd */
			if (close(pipe_fd[0]) == -1) {
				printf("Process no: %d pid: %d error: %s\n",
						i+1, getpid(),
						strerror(errno));
				exit(-1);
			}

			int event_id = 0;
			int motion_type = i%3;
			int ret_val;
			struct acc_motion *motion = (struct acc_motion *) malloc
				(sizeof(struct acc_motion));
			/*for debugging*/
			printf("I am child number: %d pid = %d\n", i+1,
					getpid());
			switch (motion_type) {
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
				motion->dlt_x = (i+1)*100;
				motion->dlt_y = (i+1)*200;
				motion->dlt_z = 0;
				break;
			default:
				printf("I don't know how child %d"
						, i+1);
				printf(" came here\n");
				exit(-1);
			}
			motion->frq = frequency;
			printf("frq selected: %d\n", motion->frq);
			/* create the event with the motion*/
			event_id = syscall(__NR_accevt_create, motion);
			if (event_id == -1) {
				printf("1. Process %d encountered error: %s\n",
						getpid(),
						strerror(errno));
				exit(-1);
			}
			if (write(pipe_fd[1], &event_id,
						sizeof(event_id)) == -1) {
				printf("2. Process %d encountered error: %s\n",
						getpid(),
						strerror(errno));
				exit(-1);
			}
			ret_val = syscall(__NR_accevt_wait, event_id);

			/* check if its -EINVAL */
			if (ret_val == -1 && errno == EINVAL) {
				printf("3. Process %d woke up",
						getpid());
				printf("but no shake detected.\n");
				exit(0);
			} else if (ret_val == 0) {
				printf("Process %d detected a ", getpid());
				switch (motion_type) {
				case 0:
					printf("horizontal shake\n");
					break;
				case 1:
					printf("vorizontal shake\n");
					break;
				case 2:
					printf("shake\n");
					break;
				default:
					printf("invalid shake\n");
				}
				exit(0);
			} else {
				printf("3. Process %d encountered error: %s\n",
						getpid(),
						strerror(errno));
				exit(-1);
			}
		} else {
			/* parent process */
			child_pid_array[i] = pid;
			/* close the write fd */
			if (close(pipe_fd[1]) == -1) {
				printf("Parent Process error: %s\n",
						strerror(errno));
				exit(-1);
			}
			if (read(pipe_fd[0], &event_id_array[i],
					sizeof(event_id_array[i])) == -1) {
				printf("Parent Process error: %s\n",
						strerror(errno));
				exit(-1);
			}
			if (close(pipe_fd[0]) == -1) {
				printf("Parent Process error: %s\n",
						strerror(errno));
				exit(-1);
			}
		}
	}

	/* sleep for sometime and close the opened events after that */
	sleep(timeout_secs);

	for (i = 0; i < num_processes; i++) {
		printf("Saved pid = %d, i = %d, event_id = %d\n",
				child_pid_array[i], i, event_id_array[i]);
		if (syscall(__NR_accevt_destroy, event_id_array[i])
				!= 0) {
			printf("During destroy error: %s\n",
					strerror(errno));
		}
	}

	return 0;
}
