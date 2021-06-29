#ifndef _LT_BASE_TIME_BASE_H_H
#define _LT_BASE_TIME_BASE_H_H

#include <limits>
#include <ostream>

#include "base/base_micro.h"
#include "base/time/time_delta.h"

namespace base {

// TimeBase--------------------------------------------------------------------

// Do not reference the time_internal::TimeBase template class directly.  Please
// use one of the time subclasses instead, and only reference the public
// TimeBase members via those classes.
namespace time_internal {

// Provides value storage and comparison/math operations common to all time
// classes. Each subclass provides for strong type-checking to ensure
// semantically meaningful comparison/math of time values from the same clock
// source or timeline.
template <class TimeClass>
class TimeBase {
public:
  // Returns true if this object has not been initialized.
  //
  // Warning: Be careful when writing code that performs math on time values,
  // since it's possible to produce a valid "zero" result that should not be
  // interpreted as a "null" value.
  constexpr bool is_null() const { return us_ == 0; }

  // Returns true if this object represents the maximum/minimum time.
  constexpr bool is_max() const { return *this == Max(); }
  constexpr bool is_min() const { return *this == Min(); }
  constexpr bool is_inf() const { return is_min() || is_max(); }

  // Returns the maximum/minimum times, which should be greater/less than than
  // any reasonable time with which we might compare it.
  static constexpr TimeClass Max() {
    return TimeClass(std::numeric_limits<int64_t>::max());
  }

  static constexpr TimeClass Min() {
    return TimeClass(std::numeric_limits<int64_t>::min());
  }

  // For legacy serialization only. When serializing to `base::Value`, prefer
  // the helpers from //base/util/values/values_util.h instead. Otherwise, use
  // `Time::ToDeltaSinceWindowsEpoch()` for `Time` and
  // `TimeDelta::InMiseconds()` for `TimeDelta`. See http://crbug.com/634507.
  constexpr int64_t ToInternalValue() const { return us_; }

  // The amount of time since the origin (or "zero") point. This is a syntactic
  // convenience to aid in code readability, mainly for debugging/testing use
  // cases.
  //
  // Warning: While the Time subclass has a fixed origin point, the origin for
  // the other subclasses can vary each time the application is restarted.
  constexpr TimeDelta since_origin() const {
    return TimeDelta::FromMicroseconds(us_);
  }

  constexpr TimeClass& operator=(TimeClass other) {
    us_ = other.us_;
    return *(static_cast<TimeClass*>(this));
  }

  // Compute the difference between two times.
  constexpr TimeDelta operator-(TimeClass other) const {
    return TimeDelta::FromMicroseconds(us_ - other.us_);
  }

  // Return a new time modified by some delta.
  constexpr TimeClass operator+(TimeDelta delta) const {
    return TimeClass(
        (TimeDelta::FromMicroseconds(us_) + delta).InMicroseconds());
  }
  constexpr TimeClass operator-(TimeDelta delta) const {
    return TimeClass(
        (TimeDelta::FromMicroseconds(us_) - delta).InMicroseconds());
  }

  // Modify by some time delta.
  constexpr TimeClass& operator+=(TimeDelta delta) {
    return static_cast<TimeClass&>(*this = (*this + delta));
  }
  constexpr TimeClass& operator-=(TimeDelta delta) {
    return static_cast<TimeClass&>(*this = (*this - delta));
  }

  // Comparison operators
  constexpr bool operator==(TimeClass other) const { return us_ == other.us_; }
  constexpr bool operator!=(TimeClass other) const { return us_ != other.us_; }
  constexpr bool operator<(TimeClass other) const { return us_ < other.us_; }
  constexpr bool operator<=(TimeClass other) const { return us_ <= other.us_; }
  constexpr bool operator>(TimeClass other) const { return us_ > other.us_; }
  constexpr bool operator>=(TimeClass other) const { return us_ >= other.us_; }

protected:
  constexpr explicit TimeBase(int64_t us) : us_(us) {}

  // Time value in a microsecond timebase.
  int64_t us_;
};
}  // namespace time_internal

template <class TimeClass>
inline constexpr TimeClass operator+(TimeDelta delta, TimeClass t) {
  return t + delta;
}

}  // namespace base

#endif
