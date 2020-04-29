#pragma once
#include <sys/types.h>

long getnstimeofday(struct timespec*);
long printdmesg(pid_t, struct timespec*, struct timespec*);


