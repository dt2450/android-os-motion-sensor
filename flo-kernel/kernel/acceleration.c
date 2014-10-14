#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>

#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/acceleration.h>
#include <linux/mylist.h>

static atomic_t counter = ATOMIC_INIT(0);
static DECLARE_WAIT_QUEUE_HEAD(__queue);
/*
 * Set current device acceleration in the kernel.
 * The parameter acceleration is the pointer to the address
 * where the sensor data is stored in user space.  Follow system call
 * convention to return 0 on success and the appropriate error value
 * on failure.
 * syscall number 378
 */

SYSCALL_DEFINE1(set_acceleration,
		struct dev_acceleration __user *, acceleration)
{
	struct dev_acceleration *k_acc = NULL;
	struct delta_elt *temp = NULL;
	int returnVal, uid;

	read_lock(&tasklist_lock);
	if (current == NULL || current->real_cred == NULL) {
		pr_err("set_acceleration: current task is invalid\n");
		read_unlock(&tasklist_lock);
		return -EFAULT;
	}

	uid = current->real_cred->uid;
	read_unlock(&tasklist_lock);

	if (uid != 0) {
		pr_err("set_acceleration: called by non-root user !!\n");
		return -EACCES;
	}

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
	if (copy_from_user(k_acc, acceleration,
				sizeof(struct dev_acceleration))) {
		pr_err("set_acceleration: copy_from_user failed.\n");
		kfree(k_acc);
		return -EFAULT;
	}

	if (delta_q_len <= WINDOW) {
		temp = kmalloc(sizeof(struct delta_elt), GFP_KERNEL);
		if (temp == NULL)
			return -ENOMEM;
	}

	add_delta_to_list(k_acc, temp);

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
	int returnVal;

	returnVal = init_event_q(1);
	if (returnVal != 0) {
		pr_err("could not initialize queue");
		return -EFAULT;
	}
	atomic_inc(&counter);
	currentEvent = kmalloc(sizeof(struct acc_motion), GFP_KERNEL);
	if (currentEvent == NULL) {
		pr_err("error: Not enough memory!");
		return -ENOMEM;
	}
	if (copy_from_user(currentEvent, acceleration,
				sizeof(struct acc_motion))) {
		pr_err("set_acceleration: copy_from_user failed.\n");
		kfree(currentEvent);
		return -EFAULT;
	}

	returnVal = add_event_to_list(currentEvent, atomic_read(&counter));
	if (returnVal == -1) {
		pr_err("could not add event to the event list");
		return -EFAULT;
	}
	return atomic_read(&counter);
}

/* Block a process on an event.
 * It takes the event_id as parameter. The event_id requires verification.
 * Return 0 on success and the appropriate error on failure.
 * system call number 380
 */

SYSCALL_DEFINE1(accevt_wait, int, event_id)
{
	struct event_elt *currentEvent = NULL;

	if (event_id <= atomic_read(&counter)) {
		read_lock(&lock_event);
		currentEvent = get_event_using_id(event_id);
		read_unlock(&lock_event);
	}
	if (currentEvent == NULL) {
		pr_err("accevt_wait: event Id not found");
		return -EFAULT;
	}
	while (!atomic_read(&(currentEvent->condition))) {
		DEFINE_WAIT(__wait);

		prepare_to_wait(&__queue, &__wait, TASK_INTERRUPTIBLE);
		if (!atomic_read(&(currentEvent->condition)))
			schedule();
		finish_wait(&__queue, &__wait);
	}
	if (atomic_read(&(currentEvent->normal_wakeup)) == 0) {
		pr_err("accevt_wait: No shake was detected\n");
		return -EINVAL;
	}
	return 0;
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
	struct delta_elt *temp = NULL;
	int dx, dy, dz, freq;
	int pid;
	int returnVal = -1;

	read_lock(&tasklist_lock);
	if (current == NULL || current->real_cred == NULL) {
		pr_err("set_acceleration: current task is invalid\n");
		read_unlock(&tasklist_lock);
		return -EFAULT;
	}

	pid = current->real_cred->uid;
	read_unlock(&tasklist_lock);

	if (pid != 0) {
		pr_err("set_acceleration: called by non-root user !!\n");
		return -EACCES;
	}

	returnVal = init_delta_q();

	if (returnVal == -1) {
		pr_err("accevt_signal: Not enough memory!");
		return -ENOMEM;
	}

	if (acceleration == NULL) {
		pr_err("accevt_signal: acceleration is NULL\n");
		return -EINVAL;
	}

	k_acc = kmalloc(sizeof(struct dev_acceleration), GFP_KERNEL);
	if (copy_from_user(k_acc,
			acceleration, sizeof(struct dev_acceleration))) {
		pr_err("accevt_signal: copy_from_user failed.\n");
		kfree(k_acc);
		return -EFAULT;
	}

	if (delta_q_len <= WINDOW) {
		temp = kmalloc(sizeof(struct delta_elt), GFP_KERNEL);
		if (temp == NULL)
			return -ENOMEM;
	}

	add_delta_to_list(k_acc, temp);

	/* Add all delta values exceeding NOISE within the current WINDOW */
	freq = add_deltas(&dx, &dy, &dz);
	if (freq == -1) {
		pr_err("accevt_signal: error occured while calculating cumulative deltas");
		kfree(k_acc);
		return -EFAULT;
	}
	returnVal = set_events_which_occurred(dx, dy, dz, freq);
	if (returnVal == -1) {
		pr_err("accevt_signal: could not set event conditions\n");
		return -EFAULT;
	}
	wake_up_all(&__queue);
	kfree(k_acc);
	return 0;
}

/* Destroy an acceleration event using the event_id,
 * Return 0 on success and the appropriate error on failure.
 * system call number 382
 */

SYSCALL_DEFINE1(accevt_destroy, int, event_id)
{
	int returnVal;
	struct event_elt *event_to_destroy;

	if (event_id <= atomic_read(&counter)) {
		write_lock(&lock_event);
		event_to_destroy = get_event_using_id(event_id);
		/*waking up processes waiting on this event
		but normal_wakeup not set hence processes will
		not print shake detected*/
		if (event_to_destroy == NULL) {
			write_unlock(&lock_event);
			pr_err("accevt_destroy: Error");
			pr_err(" while destroying event_id: %d\n",
					event_id);
			return -EINVAL;
		}
		atomic_set(&(event_to_destroy->condition), 1);
		write_unlock(&lock_event);
		wake_up_all(&__queue);
		returnVal = remove_event_using_id(event_id);
		if (returnVal == -1) {
			pr_err("accevt_destroy: Could not destroy event: %d\n",
					event_id);
			return -EFAULT;
		}
		return 0;
	}
	return -EFAULT;

}
