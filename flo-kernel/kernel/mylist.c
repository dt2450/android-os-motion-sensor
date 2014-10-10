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

static struct kfifo *delta_q;
static struct kfifo *event_q;

int init_delta_q(void)
{
	int ret;

        if (delta_q == NULL) {
		ret = kfifo_alloc(delta_q, 20 * sizeof(struct window_elt), GFP_KERNEL);
		if (ret != 0) {
			pr_err("init_event_list: could not allocate events.\n");
			return -1;
		}
		return 1;
        }
        return 0;
}

int init_event_q(void)
{
        int ret;

        if (event_q == NULL) {
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

	return 0;
}

int add_delta_to_list(struct dev_acceleration *dev_acc)
{
        unsigned int copied;

        if (delta_q == NULL) {
                pr_err("add_delta_to_list: event list is NULL.\n");
                return -1;
        }

        if (dev_acc == NULL) {
                pr_err("add_delta_to_list: motion is NULL.\n");
                return -1;
        }

        if (kfifo_is_full(delta_q)) {
                pr_err("add_delta_to_list: event_q is full\n");
                return -1;
        }

        copied = kfifo_in(delta_q, dev_acc, sizeof(struct dev_acceleration));
        if (copied != sizeof(struct dev_acceleration)) {
                pr_err("add_delta_to_list: copied only %u bytes.\n", copied);
                return -1;
        }

        return 0;
}


