#include <unistd.h>
#include <sys/types.h>
#include "kernel.h"

long getnstimeofday(struct timespec* ts)
{
	return syscall(335, ts);
}

long printdmesg(pid_t pid, struct timespec* start, struct timespec* end)
{
	return syscall(336, pid, start, end);
}
