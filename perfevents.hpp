#pragma once
#include <iostream>
#include <unistd.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <cstdlib>
#include <sys/time.h>
#include <string>
#include <cstring>

struct read_format {
	uint64_t nr;
	struct {
		uint64_t value;
		uint64_t id;
  	} values[];
};


class perfevents
{
public:

	static constexpr int NUM_EVENTS = 128;
	int fds[NUM_EVENTS];
	std::string stat_name[NUM_EVENTS];
	uint64_t ids[NUM_EVENTS];

	perfevents()
	{
	}

	void start(std::string msg)
	{
		struct perf_event_attr pe[NUM_EVENTS];
		memset(&pe, 0, sizeof(pe));
  
		fds[0] = add_event(&pe[0], PERF_TYPE_HARDWARE, PERF_COUNT_HW_REF_CPU_CYCLES, -1);
		ioctl(fds[0], PERF_EVENT_IOC_ID, &ids[0]);
		stat_name[0] = "CPU_CYCLES";

		fds[1] = add_event(&pe[1], PERF_TYPE_HW_CACHE, PERF_COUNT_HW_CACHE_LL | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16), fds[0]);
		ioctl(fds[1], PERF_EVENT_IOC_ID, &ids[1]);
		stat_name[1] = "READ.LLC.MISS";

		fds[2] = add_event(&pe[2], PERF_TYPE_HW_CACHE, PERF_COUNT_HW_CACHE_LL | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16), fds[0]);
		ioctl(fds[2], PERF_EVENT_IOC_ID, &ids[2]);
		stat_name[2] = "WRITE.LLC.MISS";

		fds[3] = add_event(&pe[3], PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, fds[0]);
		ioctl(fds[3], PERF_EVENT_IOC_ID, &ids[3]);
		stat_name[3] = "INSTRUCTIONS";

		fds[4] = add_event(&pe[4], PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES, fds[0]);
		ioctl(fds[4], PERF_EVENT_IOC_ID, &ids[4]);
		stat_name[4] = "LLC.MISSES";
    
		std::cout << "[perfevents.start] " << msg << std::endl;
		
		enable_perf(fds[0]);
	
	}

	void stop(std::string msg)
	{
		disable_perf(fds[0]);

		char buf[4096];
		struct read_format* rf = (struct read_format*) buf;
		if (read(fds[0], &buf, sizeof(buf)) == -1) {
			std::cerr << "Error reading perf counter" << std::endl;
			exit(EXIT_FAILURE);
		}

		std::cout << "[perfevents.stop] " << msg << std::endl;

		for (int i = 0; i < rf->nr; i++) {
			std::cout << "[" << stat_name[i] << "]" << " = " << rf->values[i].value << std::endl;
		}

		for (int i = 0; i < NUM_EVENTS; i++)
		{
			close(fds[i]);
		}
	}

private:
	int add_event(struct perf_event_attr* pe, uint32_t type, uint64_t config, int group_fd)
	{
		pe->type = type;
		pe->size = sizeof(struct perf_event_attr);
		pe->config = config;
		pe->disabled = 1;
		pe->exclude_kernel = 1;
		pe->exclude_hv = 1;
		pe->read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
		
		int fd = syscall(__NR_perf_event_open, pe, 0, -1, group_fd, 0);
		if (fd == -1) {
			std::cerr << "Error opening perf event" << std::endl;
			exit(EXIT_FAILURE);
		}

		return fd;
	}

	void read_perf_counter(int fd, char* buf, uint32_t len)
	{
		if (read(fd, &buf, len) == -1)
		{
			std::cerr << "Error reading perf counter" << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	void enable_perf(int fd)
	{
		ioctl(fd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
		if (ioctl(fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP) == -1)
		{
			std::cerr << "Error controlling perf event" << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	void disable_perf(int fd)
	{
		if (ioctl(fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP) == -1)
		{
			std::cerr << "Error controlling perf event" << std::endl;
			exit(EXIT_FAILURE);
		}
	}
};