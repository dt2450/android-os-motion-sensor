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

static struct kfifo *event_q = NULL;
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
        int ret;

        if (event_q == NULL) {
		event_q = kmalloc(sizeof(struct acc_motion), GFP_KERNEL);
		if (event_q == NULL) {
			pr_err("init_event_list: could not allocate event.\n");
			return -1;
		}
                ret = kfifo_alloc(event_q, 20 * sizeof(struct acc_motion), GFP_KERNEL);
                if (ret != 0) {
                        pr_err("init_event_list: could not allocate events.\n");
                        return -1;
                }
                return 1;
        }
        return 0;
}

	 
int add_event_to_list(struct acc_motion *motion)
{
	unsigned int copied;

	if (event_q == NULL) {
		pr_err("add_event_to_list: event list is NULL.\n");
		return -1;
	}

	if (motion == NULL) {
		pr_err("add_event_to_list: motion is NULL.\n");
		return -1;
	}

	if (kfifo_is_full(event_q)) {
		pr_err("add_event_to_list: event_q is full\n");
		return -1;
	}

	copied = kfifo_in(event_q, motion, sizeof(struct acc_motion));
	if (copied != sizeof(struct acc_motion)) {
		pr_err("add_event_to_list: copied only %u bytes.\n", copied);
		return -1;
	}
	event_q_len++;
	pr_info("current size of event_q: %d\n", event_q_len);
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
