#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>

#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/kfifo.h>
#include <linux/sched.h>
#include  <linux/atomic.h>
#include <linux/acceleration.h>
#include <linux/mylist.h>

int delta_q_len = 0;
DEFINE_RWLOCK(lock_delta);
DEFINE_RWLOCK(lock_event);


static struct list_head head;
static struct list_head *head_ptr;

static struct list_head delta_q_head;
static struct list_head *delta_q_head_ptr;

static int event_q_len;

static struct dev_acceleration prev;

/*This will initialize the delta queue*/
int init_delta_q(void)
{
	write_lock(&lock_delta);
	if (delta_q_len == 0) {
		pr_info("initializing delta_q_head for the 1st time\n");
		INIT_LIST_HEAD(&delta_q_head);
		delta_q_head_ptr = &delta_q_head;
		prev.x = 0;
		prev.y = 0;
		prev.z = 0;
	}

	write_unlock(&lock_delta);
	return 0;
}

/*Initializes the event queue*/
int init_event_q(void)
{
	pr_info("init_event_q: Came here 1\n");
	write_lock(&lock_event);
	pr_info("init_event_q: Came here 2\n");
	if (head_ptr == NULL || head_ptr->next == NULL || event_q_len <= 0) {
		pr_info("init_event_q: initializing event_q_head for the 1st time.\n");
		if (head_ptr == NULL)
			pr_err("init_event_q: head_ptr is NULL\n");
		else if (head_ptr->next == NULL)
			pr_err("init_event_q: head_ptr->next is NULL\n");
		pr_err("init_event_q: event_q_len: %d\n", event_q_len);
		INIT_LIST_HEAD(&head);
		head_ptr = &head;
	}

	pr_info("init_event_q: Came here 3\n");
	write_unlock(&lock_event);
	pr_info("init_event_q: Came here 4\n");
	return 0;
}

int add_event_to_list(struct acc_motion *motion, int event_id)
{
	struct event_elt *event;

	pr_info("add_event_to_list: Came here 1");
	if (motion == NULL) {
		pr_err("add_event_to_list: motion is NULL\n");
		return -1;
	}

	pr_info("add_event_to_list: Came here 2");
	event = kmalloc(sizeof(struct event_elt), GFP_KERNEL);
	if (event == NULL)
		return -1;

	pr_info("add_event_to_list: Came here 3");
	if (head_ptr == NULL) {
		pr_err("add_event_to_list: head_ptr is NULL\n");
		return -1;
	}

	pr_info("add_event_to_list: Came here 4");
	event->dx = motion->dlt_x;
	event->dy = motion->dlt_y;
	event->dz = motion->dlt_z;
	event->frq = motion->frq;
	event->id = event_id;

	pr_info("add_event_to_list: Came here 5");
	atomic_set(&(event->condition), 0);
	atomic_set(&(event->normal_wakeup), 0);


	write_lock(&lock_event);
	pr_info("add_event_to_list: Came here 6");
	pr_info("event = %x list = %x head_ptr = %x head_ptr->next= %x\n",
			(unsigned int)event, (unsigned int)&event->list,
			(unsigned int)head_ptr, (unsigned int)head_ptr->next);
	list_add(&event->list, head_ptr);
	pr_info("add_event_to_list: Came here 7");
	head_ptr = &(event->list);
	pr_info("add_event_to_list: Came here 8");
	event_q_len++;
	write_unlock(&lock_event);

	//pr_debug("Enqueued event %d: dx:%d", event_q_len, event->dx);
	//pr_debug(" dy:%d dz:%d frq:%d, ", event->dy, event->dz, event->frq);
	//pr_debug(" with id=%d\n", event->id);
	pr_info("add_event_to_list: after adding, event_q_len: %d",
			event_q_len);
	return 0;
}


/*
* Takes actual pointer to event.
* Removes the event from the list, and then FREES it,
* so don't access the event after calling this function on it.
*/
int remove_event_from_list(struct event_elt *event)
{
	/* Lock is taken by the calling function */
	pr_info("remove_event_from_list: Came here 1\n");
	if (event == NULL) {
		pr_err("remove_event_from_list: event is NULL\n");
		return -1;
	}

	pr_info("remove_event_from_list: Came here 2\n");
	if (head_ptr == NULL) {
		pr_err("remove_event_from_list: head_ptr is NULL\n");
		return -1;
	}

	pr_debug("remove_event_from_list: Going to delete event");
	pr_debug(" %d %d %d %d\n", event->dx, event->dy, event->dz, event->frq);

	if (event_q_len == 0) {
		pr_err("remove_event_from_list: event queue underflow\n");
		return -1;
	}

	list_del(&event->list);
	pr_err("remove_event_from_list: going to free the event\n");
	//kfree(event);
	event_q_len--;
	pr_err("remove_event_from_list: after removing, event_q_len=%d\n",
			event_q_len);
	if (event_q_len <= 0) {
		pr_info("remove_event_from_list: len = %d, head_ptr = %x,"
				" head_ptr_next = %x\n", event_q_len,
				(unsigned int)head_ptr,
				(unsigned int)head_ptr->next);
		init_event_q();
	}

	return 0;
}

struct event_elt *get_event_using_id(int event_id)
{
	/* lock is taken by the calling function */
	struct list_head *p;
	struct event_elt *m;

	pr_info("get_event_using_id: Came here 1\n");
	if (event_q_len == 0) {
		pr_err("get_event_using_id: No events in event_q\n");
		return NULL;
	}

	pr_info("get_event_using_id: Came here 2\n");
	if (head_ptr == NULL) {
		pr_err("get_event_using_id: event_q head_ptr is NULL\n");
		return NULL;
	}

	pr_info("get_event_using_id: Came here 3\n");
	list_for_each(p, head_ptr->next) {
		pr_info("get_event_using_id: Came here 4\n");
		m = list_entry(p, struct event_elt, list);
		if (m == NULL) {
			pr_err("get_event_using_id: retrieved NULL from event q\n");
			return NULL;
		}
		pr_info("get_event_using_id: Came here 5\n");
		if (m->id == event_id) {
			/*pr_err("get_event_using_id: found event with id");
			pr_err(" %d: %d %d %d\n",
				event_id, m->dx, m->dy, m->dz);*/
			pr_info("get_event_using_id: Came here 6\n");
			return m;
		}
	}

	pr_err("get_event_using_id: No event found with id: %d\n", event_id);
	return NULL;
}

int remove_event_using_id(int event_id)
{
	/* lock is taken by the calling function */
	struct list_head *p;
	struct event_elt *m;
	int ret, found = 0;

	pr_info("remove_event_using_id: Came here 1\n");
	if (event_q_len == 0) {
		pr_err("remove_event_using_id: No events in event_q\n");
		return -1;
	}

	pr_info("remove_event_using_id: Came here 2\n");
	if (head_ptr == NULL) {
		pr_err("remove_event_using_id: event_q head_ptr is NULL\n");
		return -1;
	}

	pr_info("remove_event_using_id: Came here 3\n");
	list_for_each(p, head_ptr->next) {
		pr_info("remove_event_using_id: Came here 4\n");
		m = list_entry(p, struct event_elt, list);
		if (m == NULL) {
			pr_err("remove_event_using_id: retrieved NULL from event q\n");
			return -1;
		}
		pr_info("remove_event_using_id: Came here 5\n");
		if (m->id == event_id) {
			pr_info("remove_event_using_id: Came here 6\n");
			found = 1;
			/*pr_err("remove_event_using_id: found event with");
			pr_err(" id %d: %d %d %d\n",
				event_id, m->dx, m->dy, m->dz);*/
			ret = remove_event_from_list(m);
			pr_info("remove_event_using_id: Came here 7\n");
			if (ret == -1)
				return -1;
			/*pr_err("remove_event_using_id:);
			pr_err("successfully removed event\n");*/
			pr_info("remove_event_using_id: Came here 8\n");
			break;
		}
	}

	pr_info("remove_event_using_id: Came here 9\n");
	if (found == 0) {
		pr_err("remove_event_using_id: No event found");
		pr_err(" with id: %d\n", event_id);
		return -1;
	}

	return 0;
}

int add_deltas(int *DX, int *DY, int *DZ)
{
	struct delta_elt *d;
	struct list_head *p;
	int i;
	int FRQ = 0;

	*DX = 0;
	*DY = 0;
	*DZ = 0;
	i = 0;

	read_lock(&lock_delta);
	if (delta_q_head_ptr == NULL) {
		pr_err("add_deltas: delta_q_head_ptr is NULL\n");
		read_unlock(&lock_delta);
		return -1;
	}

	/*pr_info("In add_deltas, summing ........\n");*/
	list_for_each(p, delta_q_head_ptr) {
		d = list_entry(p, struct delta_elt, list);
		if (d == NULL) {
			pr_err("add_deltas: retrieved NULL from delta_q\n");
			read_unlock(&lock_delta);
			return -1;
		}
		/*pr_err("Elt %d: %d %d %d %d", i, d->dx,
		d->dy, d->dz, d->frq);*/

		if (d->frq == 1) {
			*DX += d->dx;
			*DY += d->dy;
			*DZ += d->dz;
			FRQ += 1;
			i++;
		}
	}

	pr_info("Added %d deltas\n", i);

	read_unlock(&lock_delta);
	return FRQ;
}


int add_delta_to_list(struct dev_acceleration *dev_acc, struct delta_elt *temp)
{
	write_lock(&lock_delta);
	if (dev_acc == NULL) {
		pr_err("add_delta_to_list: motion is NULL.\n");
		write_unlock(&lock_delta);
		return -1;
	}

	/*if (delta_q_len >= WINDOW)*/
	if (temp == NULL) {
		pr_info("delta q is full, will pop one\n");
		temp = list_first_entry(&delta_q_head, struct delta_elt, list);
		if (temp == NULL) {
			pr_err("add_delta_to_list: could not get 1st entry.\n");
			write_unlock(&lock_delta);
			return -1;
		}
		list_del(&temp->list);
		pr_info("popped: %d %d %d\n", temp->dx, temp->dy, temp->dz);
		delta_q_len--;
	} else {
		pr_info("delta_q is not full\n");
	}
	/*for debugging*/
	pr_info("Came here 2 temp = %x dev_acc = %x\n",
			(unsigned int)temp,
			(unsigned int)dev_acc);
	temp->dx = dev_acc->x - prev.x;
	temp->dy = dev_acc->y - prev.y;
	temp->dz = dev_acc->z - prev.z;

	pr_info("Came here 3\n");
	if (temp->dx < 0)
		temp->dx = -temp->dx;
	if (temp->dy < 0)
		temp->dy = -temp->dy;
	if (temp->dz < 0)
		temp->dz = -temp->dz;

	pr_info("Came here 4\n");
	if (temp->dx + temp->dy + temp->dz > NOISE)
		temp->frq = 1;
	else
		temp->frq = 0;

	pr_info("Came here 5\n");
	prev.x = dev_acc->x;
	prev.y = dev_acc->y;
	prev.z = dev_acc->z;

	pr_info("Came here 6\n");
	list_add_tail(&temp->list, &delta_q_head);

	pr_info("Came here 7\n");
	pr_info("Pushed %d %d %d\n", temp->dx, temp->dy, temp->dz);
	delta_q_len++;
	/*pr_info("current size of delta_q: %d\n", delta_q_len);*/

	pr_info("Came here 8\n");
	write_unlock(&lock_delta);
	return 0;
}
/*
* In the caller, if status is:
* 0	: event occurred, and event_elt is returned : this is good
* 1	: no event occurred, and NULL is returned : this is also good
* -1	: error occurred, and NULL is returned : this is bad.
*/
struct event_elt **check_events_occurred(int DX, int DY, int DZ, int FRQ,
						int *status, int *len)
{
	struct list_head *p;
	struct event_elt *m;
	struct event_elt **events;
	int count, index;

	/*printk("check_events_occurred: checking stuff\n");*/
	if (head_ptr == NULL) {
		pr_err("check_event_occurred: event_q ead_ptr is NULL\n");
		*status = -1;
		return NULL;
	}

	if (head_ptr->next == NULL) {
		pr_err("check_event_occurred: event_q head_ptr->next is NULL\n");
		*status = -1;
		return NULL;
	}

	if (event_q_len == 0) {
		pr_err("check_event_occurred: No events in event_q\n");
		*status = 1;
		return NULL;
	}

	/*printk("check_events_occurred: going to start loop\n");*/
	count = 0;
	list_for_each(p, head_ptr->next) {
		/*printk("check_events_occurred: in loop\n");*/
		m = list_entry(p, struct event_elt, list);
		if (m == NULL) {
			pr_err("check_event_occurred: retrieved NULL from event q\n");
			*status = -1;
			return NULL;
		}
		/*printk("check_events_occurred: checking condition\n");*/
		if (DX >= m->dx && DY >= m->dy
				&& DZ >= m->dz && FRQ >= m->frq) {
			pr_err("check_event_occurred: found event with id");
			pr_err(" %d: %d %d %d\n", m->id, m->dx, m->dy, m->dz);
			count++;
		}
	}

	if (count == 0) {
		pr_err("check_event_occurred: No event occurred.\n");
		*status = 1;
		return NULL;
	}
	pr_err("%d events have occurred\n", count);

	events = kmalloc_array(count, sizeof(struct event_elt *), GFP_KERNEL);
	if (events == NULL) {
		*status = -1;
		return NULL;
	}
	/*printk("check_events_occurred: going in loop again\n");*/
	index = 0;
	list_for_each(p, head_ptr->next) {
		m = list_entry(p, struct event_elt, list);
		if (m == NULL) {
			pr_err("check_event_occurred: retrieved NULL from event q\n");
			*status = -1;
			return NULL;
		}
		if (DX >= m->dx && DY >= m->dy && DZ >= m->dz && FRQ >= m->frq)
			events[index++] = m;
	}

	*status = 0;
	*len = count;
	pr_err("Returning %d events\n", index);
	return events;
}
