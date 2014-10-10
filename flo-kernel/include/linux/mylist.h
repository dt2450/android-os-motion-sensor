#ifndef __LINUX_MYLIST_H
#define __LINUX_MYLIST_H

#include <linux/list.h>

struct window_elt
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
};


int init_event_q(void);
int init_delta_q(void);
int add_delta_to_list(struct dev_acceleration *dev_acc);
int add_event_to_list(struct acc_motion *motion);


#endif /* __LINUX_MYLIST_H */
