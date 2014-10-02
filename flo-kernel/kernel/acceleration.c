#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>

#include <linux/uaccess.h>
#include <linux/slab.h>

#include <linux/acceleration.h>

SYSCALL_DEFINE1(set_acceleration, struct dev_acceleration __user *, acceleration)
{
	return 2485;
}
