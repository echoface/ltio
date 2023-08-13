#include "time.h"
#include "time_ticks.h"

#include "base/memory/no_destructor.h"

namespace base {

namespace {

int64_t ConvertTimespecToMicros(const struct timespec& ts) {
  // On 32-bit systems, the calculation cannot overflow int64_t.
  // 2**32 * 1000000 + 2**64 / 1000 < 2**63
  if (sizeof(ts.tv_sec) <= 4 && sizeof(ts.tv_nsec) <= 8) {
    int64_t result = ts.tv_sec;
    result *= base::TimeDelta::kMicrosecondsPerSecond;
    result += (ts.tv_nsec / base::TimeDelta::kNanosecondsPerMicrosecond);
    return result;
  }
  base::CheckedNumeric<int64_t> result(ts.tv_sec);
  result *= base::TimeDelta::kMicrosecondsPerSecond;
  result += (ts.tv_nsec / base::TimeDelta::kNanosecondsPerMicrosecond);
  return result.ValueOrDie();
}

}  // end namespace

// static
TimeTicks TimeTicks::Now() {
  struct timespec ts;
  LTCHECK(clock_gettime(CLOCK_MONOTONIC, &ts) == 0);
  int64_t now_ticks = ConvertTimespecToMicros(ts);
  return TimeTicks() + TimeDelta::FromMicroseconds(now_ticks);
}

// static
bool TimeTicks::IsHighResolution() {
  return true;
}

// static
bool TimeTicks::IsConsistentAcrossProcesses() {
  return true;
}

// static
TimeTicks TimeTicks::UnixEpoch() {
  static const NoDestructor<TimeTicks> epoch(
      []() { return TimeTicks::Now() - (Time::Now() - Time::UnixEpoch()); }());
  return *epoch;
}

// For logging use only.
std::ostream& operator<<(std::ostream& os, TimeTicks time_ticks) {
  // This function formats a TimeTicks object as "bogo-microseconds".
  // The origin and granularity of the count are platform-specific, and may very
  // from run to run. Although bogo-microseconds usually roughly correspond to
  // real microseconds, the only real guarantee is that the number never goes
  // down during a single run.
  const TimeDelta as_time_delta = time_ticks - TimeTicks();
  return os << as_time_delta.InMicroseconds() << " bogo-microseconds";
}
}  // namespace base
