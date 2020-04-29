#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>

SYSCALL_DEFINE1(getnstimeofday, struct timespec64*, ts)
{
	struct timespec64 temp;
	ktime_get_ts64(&temp);
	return copy_to_user(ts, &temp, sizeof(struct timespec64));
}
