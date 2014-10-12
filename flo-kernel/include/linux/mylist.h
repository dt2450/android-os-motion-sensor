#ifndef __LINUX_MYLIST_H
#define __LINUX_MYLIST_H

#include <linux/acceleration.h>
#include <linux/list.h>

struct delta_elt
{
	int dx;
	int dy;
	int dz;
	int frq;

	struct list_head list;
};

struct event_elt
{
	unsigned int dx;
	unsigned int dy;
	unsigned int dz;
	unsigned int frq;

	int id;

	struct list_head list;
};

void h(void);
int init_event_q(void);
int add_event_to_list(struct acc_motion *motion, int event_id);
int remove_event_using_id(int event_id);

int init_delta_q(void);
int add_delta_to_list(struct dev_acceleration *dev_acc);
int add_deltas(int *DX, int *DY, int *DZ);

#endif /* __LINUX_MYLIST_H */
