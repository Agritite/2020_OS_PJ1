#pragma once

#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif

#include <iostream>
#include <vector>
#include <queue>
#include <list>
#include <algorithm>
#include <cstring>
#include <cerrno>
#include <ctime>
#include <csignal>

#include <unistd.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>

typedef unsigned int UINT;

struct task
{
	pid_t pid;
	char name[32];
	UINT readytime;
	UINT runtime;
	bool operator<(const task&) const;
};

struct cmdlist
{
	UINT time;
	int starttask;
	int swapin;
	int swapout;
};

bool sortbyready(const task&, const task&);

inline void unit_step();
inline void childaffinity(pid_t);

void alrm_handler(int);
void chld_handler(int);