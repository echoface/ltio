#ifndef LT_SYS_INFO_H_
#define LT_SYS_INFO_H_

#include "base/build_config.h"

#include <unistd.h>
#include <ios>
#include <string>
#include <vector>
#include <iomanip>

#ifdef OS_LINUX
#include <cerrno>
#include <sys/sysinfo.h>
#endif

namespace base {

static std::string PrettyBytes(unsigned long long bytes) {
	static std::vector<std::string> suffix = {"B", "KB", "MB", "GB", "TB"};
	char length = suffix.size();
  uint s = 0; // which suffix to use
  double count = bytes;
  while (count >= 1024 && s < length - 1) {
    s++, count /= 1024;
  }
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(2) << count << suffix[s];
	return oss.str();
}

static inline int GetNCPU() {
#ifdef OS_WIN
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  return si.dwNumberOfProcessors;
#else
  return sysconf(_SC_NPROCESSORS_CONF);  // processors configured
#endif
}

typedef struct meminfo_t {
  unsigned long long total;
  unsigned long long free;
} meminfo_t;

//in kb format
static inline int GetMemoryInfo(meminfo_t* mem) {
#if defined(OS_LINUX)
  struct sysinfo info;
  if (sysinfo(&info) < 0) {
    return errno;
  }
  mem->total = info.totalram * info.mem_unit >> 10;
  mem->free = info.freeram * info.mem_unit >> 10;
  return 0;
#elif defined(OS_DARWIN)
  uint64_t memsize = 0;
  size_t size = sizeof(memsize);
  int which[2] = {CTL_HW, HW_MEMSIZE};
  sysctl(which, 2, &memsize, &size, NULL, 0);
  mem->total = memsize >> 10;

  vm_statistics_data_t info;
  mach_msg_type_number_t count = sizeof(info) / sizeof(integer_t);
  host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&info, &count);
  mem->free = ((uint64_t)info.free_count * sysconf(_SC_PAGESIZE)) >> 10;
  return 0;
#else
  (void)(mem);
  return -1;
#endif
}

}  // namespace base

#endif
