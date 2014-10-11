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

static struct kfifo delta_q;
static int delta_q_len = 0;

static struct list_head head;
static struct list_head *head_ptr = NULL;
static int event_q_len = 0;

static struct dev_acceleration *prev = NULL;

int init_delta_q(void)
{
	int ret;
	if (delta_q_len == 0) {
		pr_info("allocating event_q for 1st time\n");
		ret = kfifo_alloc(&delta_q, 4096, GFP_KERNEL);
		if (ret != 0) {
			pr_err("init_delta_q: could not allocate events.\n");
			return -1;
		}
	}
	prev = kmalloc(sizeof(struct dev_acceleration), GFP_KERNEL);
	if (prev == NULL) {
		pr_err("init_delta_q: could not allocate prev.\n");
		return -1;
	}

	prev->x = 0;
	prev->y = 0;
	prev->z = 0;

        return 0;
}

int init_event_q(void)
{
	INIT_LIST_HEAD(&head);
        head_ptr = &head;
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

void h(void)
{
	if (!prev)
		pr_info("prev is NULL\n");
	else
		pr_info("prev is not NULL");
}

int add_delta_to_list(struct dev_acceleration *dev_acc)
{
        unsigned int copied;
	struct delta_elt *temp = NULL;

	/*
        if (delta_q == NULL) {
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

        if (delta_q_len >= 20) {
		pr_info("delta q is full, will pop one\n");
		copied = kfifo_out(&delta_q, temp, sizeof(struct delta_elt));
		if (copied != sizeof(struct delta_elt)) {
			pr_err("add_delta_to_list: copied only %u bytes.\n", copied);
			return -1;
		}
		delta_q_len--;
        } else {
		pr_info("delta_q is not full\n");
	}
	if (!temp)
		pr_info("temp is NULL\n");
	if (!prev)
		pr_info("prev is NULL");

	temp->dx = dev_acc->x - prev->x;
	temp->dy = dev_acc->y - prev->y;
	temp->dz = dev_acc->z - prev->z;

	prev->x = dev_acc->x;
	prev->y = dev_acc->y;
	prev->z = dev_acc->z;

	copied = kfifo_in(&delta_q, temp, sizeof(struct delta_elt));
	if (copied != sizeof(struct delta_elt)) {
		pr_err("add_delta_to_list: copied only %u bytes.\n", copied);
		return -1;
	}
	delta_q_len++;
	pr_info("current size of delta_q: %d\n", delta_q_len);	

        return 0;
}
