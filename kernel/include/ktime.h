#ifndef _KTIME_H_
#define _KTIME_H_

#include <sys/time.h>

/**
 * Initilize the kernel time keeping mechanism.
 */
void ktime_init(void);

/**
 * Update the current system time from the CMOS.
 */
void ktime_update(void);

/**
 * Retrieve the kernel time in seconds.
 */
time_t ktime_seconds(void);

#endif
