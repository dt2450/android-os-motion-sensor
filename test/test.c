#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <errno.h>


#define __NR_set_acceleration	378

struct dev_acceleration {
	int x; /* acceleration along X-axis */
	int y; /* acceleration along Y-axis */
	int z; /* acceleration along Z-axis */
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

	
	int r = syscall(__NR_set_acceleration, acc);

	if (r == -1) {
		printf("error : %s\n", strerror(errno));
		return -1;
	}

	printf("syscall returned: %d\n", r);
	return 0;
}
