#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>

SYSCALL_DEFINE3(printdmesg, pid_t, pid, struct timespec64*, pstart, struct timespec64*, pend)
{
	struct timespec64 start;
	struct timespec64 end;
	long ret = copy_from_user(&start, pstart, sizeof(struct timespec64));
	if(ret != 0)
		return ret;
	ret = copy_from_user(&end, pend, sizeof(struct timespec64));
	if(ret != 0)
		return -1 * ret;
	printk(KERN_INFO "[project1] %d %lld.%ld %lld.%ld\n", pid, start.tv_sec, start.tv_nsec, end.tv_sec, end.tv_nsec);
	return 0;
}
