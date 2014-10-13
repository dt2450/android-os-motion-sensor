#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>

#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/acceleration.h>
#include <linux/mylist.h>

static atomic_t *counter;

/*
 * Set current device acceleration in the kernel.
 * The parameter acceleration is the pointer to the address
 * where the sensor data is stored in user space.  Follow system call
 * convention to return 0 on success and the appropriate error value
 * on failure. 
 * syscall number 378
 */

SYSCALL_DEFINE1(set_acceleration, struct dev_acceleration __user *, acceleration)
{
	struct dev_acceleration *k_acc = NULL;
	
	int returnVal;
	
	returnVal = init_delta_q();
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
	add_delta_to_list(k_acc);

	kfree(k_acc);
	return 0;
}


/* Create an event based on motion.  
 * If frq exceeds WINDOW, cap frq at WINDOW.
 * Return an event_id on success and the appropriate error on failure.
 * system call number 379
 */
 
SYSCALL_DEFINE1(accevt_create, struct acc_motion __user *, acceleration)
{
	struct acc_motion *currentEvent = NULL;
	int returnVal = init_event_q();
	if (returnVal != 0){
		pr_err("could not initialize queue");
		return -EFAULT;
	}
	if(counter == NULL){
		counter = kmalloc(sizeof(atomic_t), GFP_KERNEL);
		atomic_set(counter,0);
	}
	atomic_inc(counter);
	currentEvent = kmalloc(sizeof(struct acc_motion), GFP_KERNEL);
	if (currentEvent == NULL) {
		pr_err("error: Not enough memory!");
		return -ENOMEM;
	}
	if (copy_from_user(currentEvent, acceleration, sizeof(struct acc_motion))) {
		pr_err("set_acceleration: copy_from_user failed.\n");
		kfree(currentEvent);
		return -EFAULT;
	}
	
	/*TODO: grab write lock on event_q*/
	if (add_event_to_list(currentEvent,atomic_read(counter)) == -1){
		pr_err("could not add event to the event list");
		return -EFAULT;
	} 
	/*TODO: release write lock on event_q*/
	return atomic_read(counter);
}
 
/* Block a process on an event.
 * It takes the event_id as parameter. The event_id requires verification.
 * Return 0 on success and the appropriate error on failure.
 * system call number 380
 */
 
SYSCALL_DEFINE1(accevt_wait, int, event_id)
{
	struct event_elt *currentEvent = NULL;
	if (event_id <= atomic_read(counter)) {
		/*get event type from the list api*/
		currentEvent = get_event_using_id(event_id);	
	}
	if (currentEvent == NULL) {
		printk("event Id not found");
		return -EFAULT;
	} else {
		/*TODO: block processes on this event id*/

		DECLARE_WAIT_QUEUE_HEAD(queue);
		DEFINE_WAIT(wait);
		while(!currentEvent->condition) {
			prepare_to_wait(&queue,&wait,TASK_INTERRUPTIBLE);
			if(!currentEvent->condition)
				schedule();
			finish_wait(&queue,&wait);
		}
		printk("x=%d, y=%d, z=%d\n", currentEvent->dx, currentEvent->dy,
		 currentEvent->dz);
	}
	return event_id;
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
	struct dev_acceleration *k_acc = NULL;
	struct event_elt **events = NULL;
	int dx, dy, dz, freq;
	int status, len, i;
	
	int returnVal = init_delta_q();
	
	if (returnVal == -1) {
		pr_err("accevt_signal: Not enough memory!");
		return -ENOMEM;
        }

	if (acceleration == NULL) {
		pr_err("accevt_signal: acceleration is NULL\n");
		return -EINVAL;
	}

	k_acc = kmalloc(sizeof(struct dev_acceleration), GFP_KERNEL);
	if (copy_from_user(k_acc, acceleration, sizeof(struct dev_acceleration))) {
		pr_err("accevt_signal: copy_from_user failed.\n");
		kfree(k_acc);
		return -EFAULT;
	}

	printk("x=%d, y=%d, z=%d\n", k_acc->x, k_acc->y, k_acc->z);
	/*TODO:grab write lock on delta_q*/
	add_delta_to_list(k_acc);
	/*TODO:release write lock on delta_q*/
	/*TODO:grab read lock on delta_q*/

	/* Add all delta values exceeding NOISE within the current WINDOW */
	freq = add_deltas(&dx,&dy,&dz);
	if (freq == -1) {
		pr_err("accevt_signal: error occured while calculating cumulative deltas");
		kfree(k_acc);
		return -EFAULT;
	}

	/* Get a list of all events that satisfy delta/frq values */
	events = check_events_occurred(dx, dy, dz, freq, &status, &len);
	if (status == -1) {
		pr_err("accevt_signal: error while checking events\n");
		kfree(k_acc);
		return -EFAULT;
	}

	if (status == 0) {
		for (i=0; i<len; i++) {
			/*TODO:  wake up processes from the queue!*/
			/* Remove the event from the event queue */
			events[i]->condition = 1;
			/*returnVal = remove_event_from_list(events[i]);
			if (returnVal == -1) {
				pr_err("accevt_signal: error while removing events\n");
				kfree(k_acc);
				return -EFAULT;
			}*/
		}
	}

	/*TODO: release read lock on delta_q*/
	kfree(k_acc);
	return 0;
}

/* Destroy an acceleration event using the event_id,
 * Return 0 on success and the appropriate error on failure.
 * system call number 382
 */

SYSCALL_DEFINE1(accevt_destroy, int, event_id)
{
	if (event_id <= atomic_read(counter)) {
		/*TODO:grab write lock on event_q*/
		int returnVal = remove_event_using_id(event_id);
		/*TODO:release write lock on event_q*/
		if(returnVal == -1){
			return -EFAULT;
		}else{
			/*TODO:destroy processes waiting on this event*/
			return 0;
		}
	} else {
		return -EFAULT;
	}
}
