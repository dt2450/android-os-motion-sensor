#ifndef __LINUX_MYLIST_H
#define __LINUX_MYLIST_H

#include <linux/acceleration.h>
#include <linux/list.h>

extern int delta_q_len;
extern rwlock_t lock_delta;
extern rwlock_t lock_event;

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
	atomic_t condition;
	atomic_t normal_wakeup;

	struct list_head list;
};

int init_event_q(void);
int add_event_to_list(struct acc_motion *motion, int event_id);
int remove_event_from_list(struct event_elt *event);
int remove_event_using_id(int event_id);
struct event_elt **check_events_occurred(int DX, int DY, int DZ, int FRQ, int *status, int *len);
int update_wait_ptr(void *wait_ptr, int pid);
struct event_elt *get_event_using_id(int event_id);

int init_delta_q(void);
int add_delta_to_list(struct dev_acceleration *dev_acc, struct delta_elt *temp);
int add_deltas(int *DX, int *DY, int *DZ);

#endif /* __LINUX_MYLIST_H */
