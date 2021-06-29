#include "time_delta.h"

#include "time.h"

namespace base {

// TimeDelta ------------------------------------------------------------------

// static
TimeDelta TimeDelta::FromTimeSpec(const timespec& ts) {
  return TimeDelta(ts.tv_sec * kMicrosecondsPerSecond +
                   ts.tv_nsec / kNanosecondsPerMicrosecond);
}

struct timespec TimeDelta::ToTimeSpec() const {
  int64_t microseconds = InMicroseconds();
  time_t seconds = 0;
  if (microseconds >= kMicrosecondsPerSecond) {
    seconds = InSeconds();
    microseconds -= seconds * kMicrosecondsPerSecond;
  }
  struct timespec result = {
      seconds, static_cast<long>(microseconds * kNanosecondsPerMicrosecond)};
  return result;
}

int TimeDelta::InDays() const {
  if (!is_inf())
    return static_cast<int>(delta_ / kMicrosecondsPerDay);
  return (delta_ < 0) ? std::numeric_limits<int>::min()
                      : std::numeric_limits<int>::max();
}

int TimeDelta::InDaysFloored() const {
  if (!is_inf()) {
    const int result = delta_ / kMicrosecondsPerDay;
    // Convert |result| from truncating to flooring.
    return (result * kMicrosecondsPerDay > delta_) ? (result - 1) : result;
  }
  return (delta_ < 0) ? std::numeric_limits<int>::min()
                      : std::numeric_limits<int>::max();
}

double TimeDelta::InMillisecondsF() const {
  if (!is_inf())
    return double(delta_) / kMicrosecondsPerMillisecond;
  return (delta_ < 0) ? -std::numeric_limits<double>::infinity()
                      : std::numeric_limits<double>::infinity();
}

int64_t TimeDelta::InMilliseconds() const {
  if (!is_inf())
    return delta_ / kMicrosecondsPerMillisecond;
  return (delta_ < 0) ? std::numeric_limits<int64_t>::min()
                      : std::numeric_limits<int64_t>::max();
}

int64_t TimeDelta::InMillisecondsRoundedUp() const {
  if (!is_inf()) {
    const int64_t result = delta_ / kMicrosecondsPerMillisecond;
    // Convert |result| from truncating to ceiling.
    return (delta_ > result * kMicrosecondsPerMillisecond) ? (result + 1)
                                                           : result;
  }
  return delta_;
}

double TimeDelta::InMicrosecondsF() const {
  if (!is_inf())
    return double(delta_);
  return (delta_ < 0) ? -std::numeric_limits<double>::infinity()
                      : std::numeric_limits<double>::infinity();
}

TimeDelta TimeDelta::CeilToMultiple(TimeDelta interval) const {
  if (is_inf() || interval.is_zero())
    return *this;
  const TimeDelta remainder = *this % interval;
  if (delta_ < 0)
    return *this - remainder;
  return remainder.is_zero() ? *this
                             : (*this - remainder + interval.magnitude());
}

TimeDelta TimeDelta::FloorToMultiple(TimeDelta interval) const {
  if (is_inf() || interval.is_zero())
    return *this;
  const TimeDelta remainder = *this % interval;
  if (delta_ < 0) {
    return remainder.is_zero() ? *this
                               : (*this - remainder - interval.magnitude());
  }
  return *this - remainder;
}

TimeDelta TimeDelta::RoundToMultiple(TimeDelta interval) const {
  if (is_inf() || interval.is_zero())
    return *this;
  if (interval.is_inf())
    return TimeDelta();
  const TimeDelta half = interval.magnitude() / 2;
  return (delta_ < 0) ? (*this - half).CeilToMultiple(interval)
                      : (*this + half).FloorToMultiple(interval);
}

std::ostream& operator<<(std::ostream& os, TimeDelta time_delta) {
  return os << time_delta.InSecondsF() << " s";
}

}  // namespace base
