#ifndef __LINUX_MYLIST_H
#define __LINUX_MYLIST_H

#include <linux/acceleration.h>
#include <linux/list.h>

struct delta_elt
{
	int dx;
	int dy;
	int dz;
};

struct event_elt
{
	unsigned int dx;
	unsigned int dy;
	unsigned int dz;
	unsigned int frq;

	struct list_head list;
};

void h(void);
int init_event_q(void);
int add_event_to_list(struct acc_motion *motion);
int remove_event_from_list(struct event_elt *event);

int init_delta_q(void);
int add_delta_to_list(struct dev_acceleration *dev_acc);

#endif /* __LINUX_MYLIST_H */
