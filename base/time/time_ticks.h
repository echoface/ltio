#ifndef _LT_BASE_TIME_TICKS_H_H
#define _LT_BASE_TIME_TICKS_H_H

#include "time_base.h"

namespace base {

// TimeTicks ------------------------------------------------------------------

// Represents monotonically non-decreasing clock time.
class TimeTicks : public time_internal::TimeBase<TimeTicks> {
public:
  constexpr TimeTicks() : TimeBase(0) {}

  // Platform-dependent tick count representing "right now." When
  // IsHighResolution() returns false, the resolution of the clock could be
  // as coarse as ~15.6ms. Otherwise, the resolution should be no worse than one
  // microsecond.
  static TimeTicks Now();

  // Returns true if the high resolution clock is working on this system and
  // Now() will return high resolution values. Note that, on systems where the
  // high resolution clock works but is deemed inefficient, the low resolution
  // clock will be used instead.
  static bool IsHighResolution() WARN_UNUSED_RESULT;

  // Returns true if TimeTicks is consistent across processes, meaning that
  // timestamps taken on different processes can be safely compared with one
  // another. (Note that, even on platforms where this returns true, time values
  // from different threads that are within one tick of each other must be
  // considered to have an ambiguous ordering.)
  static bool IsConsistentAcrossProcesses() WARN_UNUSED_RESULT;

#if defined(OS_MAC)
  static TimeTicks FromMachAbsoluteTime(uint64_t mach_absolute_time);

  static mach_timebase_info_data_t* MachTimebaseInfo();
#endif  // defined(OS_MAC)

#if defined(OS_ANDROID)
  // Converts to TimeTicks the value obtained from SystemClock.uptimeMillis().
  // Note: this convertion may be non-monotonic in relation to previously
  // obtained TimeTicks::Now() values because of the truncation (to
  // milliseconds) performed by uptimeMillis().
  static TimeTicks FromUptimeMillis(int64_t uptime_millis_value);
#endif

  // Get an estimate of the TimeTick value at the time of the UnixEpoch. Because
  // Time and TimeTicks respond differently to user-set time and NTP
  // adjustments, this number is only an estimate. Nevertheless, this can be
  // useful when you need to relate the value of TimeTicks to a real time and
  // date. Note: Upon first invocation, this function takes a snapshot of the
  // realtime clock to establish a reference point.  This function will return
  // the same value for the duration of the application, but will be different
  // in future application runs.
  static TimeTicks UnixEpoch();

  // Converts an integer value representing TimeTicks to a class. This may be
  // used when deserializing a |TimeTicks| structure, using a value known to be
  // compatible. It is not provided as a constructor because the integer type
  // may be unclear from the perspective of a caller.
  //
  // DEPRECATED - Do not use in new code. For deserializing TimeTicks values,
  // prefer TimeTicks + TimeDelta(); however, be aware that the origin is not
  // fixed and may vary. Serializing for persistence is strongly discouraged.
  // http://crbug.com/634507
  static constexpr TimeTicks FromInternalValue(int64_t us) {
    return TimeTicks(us);
  }

private:
  friend class time_internal::TimeBase<TimeTicks>;

  // Please use Now() to create a new object. This is for internal use
  // and testing.
  constexpr explicit TimeTicks(int64_t us) : TimeBase(us) {}
};

std::ostream& operator<<(std::ostream& os, TimeTicks time_ticks);

}  // namespace base
#endif
