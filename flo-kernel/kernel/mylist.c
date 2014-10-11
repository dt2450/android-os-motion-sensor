#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>

#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/kfifo.h>

#include <linux/acceleration.h>
#include <linux/mylist.h>

static int delta_q_len = 0;

static struct list_head head;
static struct list_head *head_ptr = NULL;

static struct list_head delta_q_head;
static struct list_head *delta_q_head_ptr = NULL;

static int event_q_len = 0;

static struct dev_acceleration *prev = NULL;

/*This will initialize the delta queue*/
int init_delta_q(void)
{
	if (delta_q_len == 0) {
		pr_info("initializing delta_q_head for the 1st time\n");
		INIT_LIST_HEAD(&delta_q_head);
		delta_q_head_ptr = &delta_q_head;
	}

	if (prev == NULL) {
		prev = kmalloc(sizeof(struct dev_acceleration), GFP_KERNEL);
		if (prev == NULL) {
			pr_err("init_delta_q: could not allocate prev.\n");
			return -1;
		}

		prev->x = 0;
		prev->y = 0;
		prev->z = 0;
	}

        return 0;
}

/*Initializes the event queue*/
int init_event_q(void)
{
	if (event_q_len == 0) {
		pr_info("initializing event_q_head for the 1st time.\n");
		INIT_LIST_HEAD(&head);
		head_ptr = &head;
	}

	return 0;
}

	 
int add_event_to_list(struct acc_motion *motion)
{
	struct event_elt *event;
	
	if (motion == NULL) {
		pr_err("add_event_to_list: motion is NULL\n");
		return -1;
	}

	event = kmalloc(sizeof(struct event_elt), GFP_KERNEL);
	if (event == NULL) {
		pr_err("add_event_to_list: could not allocate event\n");
		return -1;
	}

	if (head_ptr == NULL) {
		pr_err("add_event_to_list: head_ptr is NULL\n");
		return -1;
	}
	
        event->dx = motion->dlt_x;
        event->dy = motion->dlt_y;
        event->dz = motion->dlt_z;
        event->frq = motion->frq;
        
	list_add(&event->list, head_ptr);
        head_ptr = &(event->list);
        
	event_q_len++;
	printk("enqueued %d: %d %d %d %d\n", event_q_len, event->dx, event->dy, event->dz, event->frq);
	return 0;
}

int remove_event_from_list(struct event_elt *event)
{
        struct list_head *p;
        struct event_elt *m;
        int i;
	
	if (event == NULL) {
		pr_err("remove_event_from_list: event is NULL\n");
		return -1;
	}

	if (head_ptr == NULL) {
		pr_err("remove_event_from_list: head_ptr is NULL\n");
		return -1;
	}

        printk("Going to delete event %d %d %d %d\n", event->dx, event->dy, event->dz, event->frq);

        if (event_q_len == 0) {
                pr_err("remove_event_from_list: event queue underflow\n");
                return -1;
        }

        //printk("before delete, head_ptr: %x, &event->list= %x\n", (unsigned int)head_ptr, (unsigned int)&event->list);
        list_del(&event->list);
	event_q_len--;
        i = 0;
        list_for_each(p, head_ptr->next) {
                m = list_entry(p, struct event_elt, list);
                printk("Element %d: %d\n", i, m->dx);
                i++;
        }
        //printk("after delete, head_ptr: %x, &event->list= %x\n", (unsigned int)head_ptr, (unsigned int)&event->list);

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

	pr_info("In add_deltas, summing ........\n");
	list_for_each(p, delta_q_head_ptr) {
		d = list_entry(p, struct delta_elt, list);
		pr_info("Elt %d: %d %d %d %d", i, d->dx, d->dy, d->dz, d->frq);
		*DX += d->dx;
		*DY += d->dy;
		*DZ += d->dz;
		if (d->frq == 1)
			FRQ += 1;
		i++;
	}
	
	pr_info("Added %d deltas\n", i);

	return FRQ;
}


int add_delta_to_list(struct dev_acceleration *dev_acc)
{
	struct delta_elt *temp = NULL;
	int DX, DY, DZ, FRQ;

	/*
        if (delta_q_head_ptr == NULL) {
                pr_err("add_delta_to_list: event list is NULL.\n");
                return -1;
        }
	*/

        if (dev_acc == NULL) {
                pr_err("add_delta_to_list: motion is NULL.\n");
                return -1;
        }
	
	temp = kmalloc(sizeof(struct delta_elt), GFP_KERNEL);
	if (temp == NULL) {
		pr_err("add_delta_to_list: can't allocate temp.\n");
		return -1;
	}

	if (prev == NULL) {
		pr_err("add_delta_to_list: prev is NULL.\n");
		return -1;
	}

        if (delta_q_len >= 20) {
		pr_info("delta q is full, will pop one\n");
		temp = list_first_entry(&delta_q_head, struct delta_elt, list);
		if (temp == NULL) {
			pr_err("add_delta_to_list: could not get 1st entry.\n");
			return -1;
		}
		list_del(&temp->list);
		pr_info("popped: %d %d %d\n", temp->dx, temp->dy, temp->dz);
		delta_q_len--;
        } else {
		pr_info("delta_q is not full\n");
	}

	temp->dx = dev_acc->x - prev->x;
	temp->dy = dev_acc->y - prev->y;
	temp->dz = dev_acc->z - prev->z;

	if (temp->dx < 0) temp->dx = -temp->dx;
	if (temp->dy < 0) temp->dy = -temp->dy;
	if (temp->dz < 0) temp->dz = -temp->dz;

	if (temp->dx + temp->dy + temp->dz > NOISE)
		temp->frq = 1;

	prev->x = dev_acc->x;
	prev->y = dev_acc->y;
	prev->z = dev_acc->z;

	list_add_tail(&temp->list, &delta_q_head);
	
	pr_info("Pushed %d %d %d\n", temp->dx, temp->dy, temp->dz);
	delta_q_len++;
	pr_info("current size of delta_q: %d\n", delta_q_len);

	
	FRQ = add_deltas(&DX, &DY, &DZ);
	if (FRQ == -1) {
		return -1;
	}

	pr_info("Current DX=%d, DY=%d, DZ=%d, FRQ=%d\n", DX, DY, DZ, FRQ);
	

        return 0;
}
