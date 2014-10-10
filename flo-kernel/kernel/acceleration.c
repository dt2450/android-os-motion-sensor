#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>

#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/acceleration.h>
#include <linux/mylist.h>

/*
 * Set current device acceleration in the kernel.
 * The parameter acceleration is the pointer to the address
 * where the sensor data is stored in user space.  Follow system call
 * convention to return 0 on success and the appropriate error value
 * on failure. 
 * syscall number 378
 */

static struct acc_motion **k_acc_motion = NULL;
static int counter;

static int init_event_list(){
	return 0;
}

SYSCALL_DEFINE1(set_acceleration, struct dev_acceleration __user *, acceleration)
{
	struct dev_acceleration *k_acc = NULL;
	
	int returnVal = init_event_list();
	if (returnVal == -1) {
		pr_err("error: Not enough memory!");
		return -ENOMEM;
	}
	if (acceleration == NULL) {
		pr_err("set_acceleration: acceleration is NULL\n");
		return -EINVAL;
	}

	k_acc = kmalloc(sizeof(struct dev_acceleration), GFP_KERNEL);
	if (copy_from_user(k_acc, acceleration, sizeof(struct dev_acceleration))) {
		pr_err("set_acceleration: copy_from_user failed.\n");
		kfree(k_acc);
		return -EFAULT;
	}

	printk("x=%d, y=%d, z=%d\n", k_acc->x, k_acc->y, k_acc->z);
	add_to_window(k_acc);
	printk("Window has %d entries.\n", window_index+1);

	kfree(k_acc);
	return 378;
}


/* Create an event based on motion.  
 * If frq exceeds WINDOW, cap frq at WINDOW.
 * Return an event_id on success and the appropriate error on failure.
 * system call number 379
 */
 
SYSCALL_DEFINE1(accevt_create, struct acc_motion __user *, acceleration)
{	counter += 1;
	k_acc_motion = krealloc(k_acc_motion, (counter * sizeof(struct acc_motion *)),
				 GFP_KERNEL);
	if (k_acc_motion == NULL ) {
		pr_err("error: Not enough memory!");
		return -ENOMEM;
	}
	struct acc_motion *currentEvent = kmalloc(sizeof(struct acc_motion), GFP_KERNEL);
	if (currentEvent == NULL) {
		pr_err("error: Not enough memory!");
		return -ENOMEM;
	}
	if (copy_from_user(acc_motion, acceleration, sizeof(struct acc_motion))) {
		pr_err("set_acceleration: copy_from_user failed.\n");
		kfree(acc_motion);
		return -EFAULT;
	}
	
	*(k_acc_motion + counter - 1) = currentEvent;
	
	return counter;
}
 
/* Block a process on an event.
 * It takes the event_id as parameter. The event_id requires verification.
 * Return 0 on success and the appropriate error on failure.
 * system call number 380
 */
 
SYSCALL_DEFINE1(accevt_wait, int, event_id)
{
	return 380;
}
 
 
/* The acc_signal system call
 * takes sensor data from user, stores the data in the kernel,
 * generates a motion calculation, and notify all open events whose
 * baseline is surpassed.  All processes waiting on a given event 
 * are unblocked.
 * Return 0 success and the appropriate error on failure.
 * system call number 381
 */
 
SYSCALL_DEFINE1(accevt_signal, struct dev_acceleration __user *, acceleration)
{
	return 381;
}

/* Destroy an acceleration event using the event_id,
 * Return 0 on success and the appropriate error on failure.
 * system call number 382
 */
 
SYSCALL_DEFINE1(accevt_destroy, int, event_id)
{
	return 382;
}
