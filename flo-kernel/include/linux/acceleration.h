#ifndef __LINUX_ACCELERATION_H
#define __LINUX_ACCELERATION_H

/*
 *Define time interval (ms)
 */
#define TIME_INTERVAL  200

/*
 * The data structure to be used for passing accelerometer data to the
 * kernel and storing the data in the kernel.
 */
struct dev_acceleration
{
	int x; /* acceleration along X-axis */
	int y; /* acceleration along Y-axis */
	int z; /* acceleration along Z-axis */
}; 

#endif /* __LINUX_ACCELERATION_H */
