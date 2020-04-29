#include "pssched.h"
#include "kernel.h"

// global

volatile sig_atomic_t alrm_flag = 1;
volatile sig_atomic_t chld_flag = 0;
volatile sig_atomic_t timer_intv_count = 0;

constexpr UINT RRunit = 500;

int main()
{
	struct sched_param sched_param_min, sched_param_max;
	sched_param_min.sched_priority = sched_get_priority_min(SCHED_FIFO);
	sched_param_max.sched_priority = sched_get_priority_max(SCHED_FIFO);

	set_cpu0:
	{
		cpu_set_t mask;
		CPU_ZERO(&mask);
		CPU_SET(0, &mask);
		sched_setaffinity(0, sizeof(cpu_set_t), &mask);
		sched_setscheduler(0, SCHED_FIFO, &sched_param_min);
	}

	char schedulepolicy[5];
	UINT numprocess;
	std::cin >> schedulepolicy;
	int policy = -1; //FIFO=0 PSJF=1 SJF=2 RR=3
	if(strcmp(schedulepolicy, "FIFO") == 0)
		policy = 0;
	else if(strcmp(schedulepolicy, "PSJF") == 0)
		policy = 1;
	else if(strcmp(schedulepolicy, "SJF") == 0)
		policy = 2;
	else if(strcmp(schedulepolicy, "RR") == 0)
		policy = 3;

	std::cin >> numprocess;
	task* tasklist = (task*)mmap(NULL, numprocess * sizeof(task), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if(tasklist < 0)
	{
		std::cerr << "mmap\n";
		return 0;
	}
	for(UINT i = 0 ; i < numprocess ; i++)
	{
		std::cin >>
		tasklist[i].name >>
		tasklist[i].readytime >>
		tasklist[i].runtime;
		tasklist[i].pid = -1;
	}

	std::sort(tasklist, tasklist + numprocess, sortbyready);
	for(size_t i = 0 ; i < numprocess ; i++)
		tasklist[i].pid = i;
	std::list<cmdlist> timeline;

	if((policy == 0) || (policy == 3))//FIFO & RR
	{
		std::queue<task> taskqueue;
		task running;
		running.pid = -1;
		for(UINT time = 0, i = 0 ; (i < numprocess) || (taskqueue.size() != 0) ; time += RRunit)
		{
			for(; (i < numprocess) && (tasklist[i].readytime <= time) ; i++)
			{
				taskqueue.push(tasklist[i]);
				cmdlist _this;
				_this.time = tasklist[i].readytime;
				_this.starttask = i;
				_this.swapin = _this.swapout = -1;
				timeline.push_back(_this);
			}
			if((running.pid == -1) || (running.runtime == 0))
			{
				if(taskqueue.size() == 0)
					running.pid = -1;
				else
				{
					running = taskqueue.front();
					taskqueue.pop();
					cmdlist newcmd;
					newcmd.time = time;
					newcmd.starttask = newcmd.swapout = -1;
					newcmd.swapin = running.pid;
					timeline.push_back(newcmd);
				}	
			}
			else if (policy == 3)
			{
				cmdlist newcmd;
				taskqueue.push(running);
				newcmd.swapout = running.pid;
				running = taskqueue.front();
				taskqueue.pop();
				newcmd.time = time;
				newcmd.starttask = -1;
				newcmd.swapin = running.pid;
				timeline.push_back(newcmd);
			}
			if(running.runtime <= RRunit)
				running.runtime = 0;
			else
				running.runtime -= RRunit;
		}
	}
	else if((policy == 1) || (policy == 2))//PSJF & SJF
	{
		std::priority_queue<task> minheap;
		task running;
		running.pid = -1;
		for(UINT time = 0, i = 0 ; (i < numprocess) || (minheap.size() != 0) ; time++)
		{
			for(; (i < numprocess) && (tasklist[i].readytime == time) ; i++)
			{
				minheap.push(tasklist[i]);
				cmdlist _this;
				_this.time = time;
				_this.starttask = i;
				_this.swapin = _this.swapout = -1;
				timeline.push_back(_this);
			}
			if((running.pid == -1) || (running.runtime == 0))
			{
				if(minheap.size() == 0)
					running.pid = -1;
				else
				{
					running = minheap.top();
					minheap.pop();
					cmdlist newcmd;
					newcmd.time = time;
					newcmd.starttask = newcmd.swapout = -1;
					newcmd.swapin = running.pid;
					timeline.push_back(newcmd);
				}
			}
			else if((policy == 1) && (minheap.top().runtime < running.runtime))
			{
				cmdlist newcmd;
				minheap.push(running);
				newcmd.swapout = running.pid;
				running = minheap.top();
				minheap.pop();
				newcmd.time = time;
				newcmd.starttask = -1;
				newcmd.swapin = running.pid;
				timeline.push_back(newcmd);
			}
			running.runtime--;
		}
	}

	#ifdef DEBUG
	for(UINT i = 0 ; i < numprocess ; i++)
	{
		std::cerr <<
		tasklist[i].name << ' ' <<
		tasklist[i].readytime << ' ' <<
		tasklist[i].runtime << '\n';
	}
	for(auto it : timeline)
		std::cerr << "time = " << it.time << '\n' <<
					"starttask = " << ((it.starttask < 0) ? "-1" : tasklist[it.starttask].name) << '\n' <<
					"swapin = " << ((it.swapin < 0) ? "-1" : tasklist[it.swapin].name) << '\n' <<
					"swapout = " << ((it.swapout < 0) ? "-1" : tasklist[it.swapout].name) << "\n\n";
	#endif

	set_signal:
	{
		struct sigaction act;
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_NOCLDSTOP;
		act.sa_handler = &chld_handler;
		sigaction(SIGCHLD, &act, NULL);
	}

	auto it = timeline.cbegin();
	for(UINT endedps = 0 ; endedps < numprocess ; )
	{
		unit_step();
		{
			#ifdef DEBUG
			std::cerr << "SIGALRM fired at intv = " << timer_intv_count << '\n';
			#endif

			while((it != timeline.cend()) && (it->time == timer_intv_count))
			{
				if(it->starttask >= 0)
				{
					UINT thisrt = tasklist[it->starttask].runtime;
					#ifdef DEBUG
					std::cerr << "child forked at intv = " << timer_intv_count << '\n';
					#endif
					pid_t pid_child = fork();
					if(pid_child != 0)
					{
						childaffinity(pid_child);
						tasklist[it->starttask].pid = pid_child;
					}
					else
					{
						struct timespec startns;
						getnstimeofday(&startns);

						for(volatile UINT i = 0 ; i < thisrt ; i++)
							unit_step();

						struct timespec endns;
						getnstimeofday(&endns);
						printdmesg(getpid(), &startns, &endns);
						return 0;
					}
				}
				if(it->swapout >= 0)
					sched_setparam(tasklist[it->swapout].pid, &sched_param_min);
				if(it->swapin >= 0)
					sched_setparam(tasklist[it->swapin].pid, &sched_param_max);
				it++;
			}
			timer_intv_count++;
		}
		if(chld_flag)
		{
			#ifdef DEBUG
			std::cerr << "SIGCHLD fired at endedps = " << endedps << '\n';
			#endif
			while(true)
			{
				pid_t reaped = waitpid(-1, NULL, WNOHANG);
				if(reaped > 0)
					endedps++;
				else
					break;
			}
			chld_flag = 0;
		}
	}
	for(UINT i = 0 ; i < numprocess ; i++)
		std::cout << tasklist[i].name << ' ' << tasklist[i].pid << '\n';
	
	munmap(tasklist, numprocess * sizeof(task));
	return 0;
}

void childaffinity(pid_t pid_child)
{
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(1, &mask);
	sched_setaffinity(pid_child, sizeof(cpu_set_t), &mask);
}

void unit_step()
{ 
	volatile UINT i;
	for(i=0 ; i<1000000UL ; i++);
}

void chld_handler(int signal)
{
	chld_flag = 1;
}

bool sortbyready(const task& a, const task& b)
{
	UINT pa = a.readytime, pb = b.readytime;
	if(pa < pb) 
		return true;
	else if(pa == pb) 
		return (strcmp(a.name, b.name) < 0);
	else
		return false;
}

bool task::operator<(const task& rhs) const
{
	UINT pa = this->runtime, pb = rhs.runtime;
	if(pa < pb) 
		return false;
	else if(pa == pb) 
		return (strcmp(this->name, rhs.name) >= 0);
	else
		return true;
}