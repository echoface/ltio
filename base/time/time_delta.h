#ifndef _LT_BASE_TIME_DURATION_H_H
#define _LT_BASE_TIME_DURATION_H_H

#include <limits>
#include <ostream>

#include "base/base_micro.h"
#include "base/check.h"
#include "base/numerics/clamped_math.h"

namespace base {

class TimeDelta {
public:
  static constexpr int64_t kHoursPerDay = 24;
  static constexpr int64_t kSecondsPerMinute = 60;
  static constexpr int64_t kMinutesPerHour = 60;
  static constexpr int64_t kSecondsPerHour =
      kSecondsPerMinute * kMinutesPerHour;
  static constexpr int64_t kMillisecondsPerSecond = 1000;
  static constexpr int64_t kMillisecondsPerDay =
      kMillisecondsPerSecond * kSecondsPerHour * kHoursPerDay;
  static constexpr int64_t kMicrosecondsPerMillisecond = 1000;
  static constexpr int64_t kMicrosecondsPerSecond =
      kMicrosecondsPerMillisecond * kMillisecondsPerSecond;
  static constexpr int64_t kMicrosecondsPerMinute =
      kMicrosecondsPerSecond * kSecondsPerMinute;
  static constexpr int64_t kMicrosecondsPerHour =
      kMicrosecondsPerMinute * kMinutesPerHour;
  static constexpr int64_t kMicrosecondsPerDay =
      kMicrosecondsPerHour * kHoursPerDay;
  static constexpr int64_t kMicrosecondsPerWeek = kMicrosecondsPerDay * 7;
  static constexpr int64_t kNanosecondsPerMicrosecond = 1000;
  static constexpr int64_t kNanosecondsPerSecond =
      kNanosecondsPerMicrosecond * kMicrosecondsPerSecond;

  constexpr TimeDelta() = default;

  // Converts units of time to TimeDeltas.
  // These conversions treat minimum argument values as min type values or -inf,
  // and maximum ones as max type values or +inf; and their results will produce
  // an is_min() or is_max() TimeDelta. WARNING: Floating point arithmetic is
  // such that FromXXXD(t.InXXXF()) may not precisely equal |t|. Hence, floating
  // point values should not be used for storage.
  static constexpr TimeDelta FromDays(int days);
  static constexpr TimeDelta FromHours(int hours);
  static constexpr TimeDelta FromMinutes(int minutes);
  static constexpr TimeDelta FromSecondsD(double secs);
  static constexpr TimeDelta FromSeconds(int64_t secs);
  static constexpr TimeDelta FromMilliseconds(int64_t ms);
  static constexpr TimeDelta FromMicroseconds(int64_t us);
  static constexpr TimeDelta FromNanoseconds(int64_t ns);

  static TimeDelta FromTimeSpec(const timespec& ts);

  // Converts a frequency in Hertz (cycles per second) into a period.
  static constexpr TimeDelta FromHz(double frequency);

  // Converts an integer value representing TimeDelta to a class. This is used
  // when deserializing a |TimeDelta| structure, using a value known to be
  // compatible. It is not provided as a constructor because the integer type
  // may be unclear from the perspective of a caller.
  //
  // DEPRECATED - Do not use in new code. http://crbug.com/634507
  static constexpr TimeDelta FromInternalValue(int64_t delta) {
    return TimeDelta(delta);
  }

  // Returns the maximum time delta, which should be greater than any reasonable
  // time delta we might compare it to. Adding or subtracting the maximum time
  // delta to a time or another time delta has an undefined result.
  static constexpr TimeDelta Max();

  // Returns the minimum time delta, which should be less than than any
  // reasonable time delta we might compare it to. Adding or subtracting the
  // minimum time delta to a time or another time delta has an undefined result.
  static constexpr TimeDelta Min();

  // Returns the maximum time delta which is not equivalent to infinity. Only
  // subtracting a finite time delta from this time delta has a defined result.
  static constexpr TimeDelta FiniteMax();

  // Returns the minimum time delta which is not equivalent to -infinity. Only
  // adding a finite time delta to this time delta has a defined result.
  static constexpr TimeDelta FiniteMin();

  // Returns the internal numeric value of the TimeDelta object. Please don't
  // use this and do arithmetic on it, as it is more error prone than using the
  // provided operators.
  // For serializing, use FromInternalValue to reconstitute.
  //
  // DEPRECATED - Do not use in new code. http://crbug.com/634507
  constexpr int64_t ToInternalValue() const { return delta_; }

  // Returns the magnitude (absolute value) of this TimeDelta.
  constexpr TimeDelta magnitude() const {
    // The code below will not work correctly in this corner case.
    if (is_min())
      return Max();

    // std::abs() is not currently constexpr.  The following is a simple
    // branchless implementation:
    const int64_t mask = delta_ >> (sizeof(delta_) * 8 - 1);
    return TimeDelta((delta_ + mask) ^ mask);
  }

  // Returns true if the time delta is zero.
  constexpr bool is_zero() const { return delta_ == 0; }

  // Returns true if the time delta is the maximum/minimum time delta.
  constexpr bool is_max() const { return *this == Max(); }
  constexpr bool is_min() const { return *this == Min(); }
  constexpr bool is_inf() const { return is_min() || is_max(); }

  struct timespec ToTimeSpec() const;

  // Returns the frequency in Hertz (cycles per second) that has a period of
  // *this.
  constexpr double ToHz() const { return FromSeconds(1) / *this; }

  // Returns the time delta in some unit. Minimum argument values return as
  // -inf for doubles and min type values otherwise. Maximum ones are treated as
  // +inf for doubles and max type values otherwise. Their results will produce
  // an is_min() or is_max() TimeDelta. The InXYZF versions return a floating
  // point value. The InXYZ versions return a truncated value (aka rounded
  // towards zero, std::trunc() behavior). The InXYZFloored() versions round to
  // lesser integers (std::floor() behavior). The XYZRoundedUp() versions round
  // up to greater integers (std::ceil() behavior). WARNING: Floating point
  // arithmetic is such that FromXXXD(t.InXXXF()) may not precisely equal |t|.
  // Hence, floating point values should not be used for storage.
  int InDays() const;
  int InDaysFloored() const;
  constexpr int InHours() const;
  constexpr int InMinutes() const;
  constexpr double InSecondsF() const;
  constexpr int64_t InSeconds() const;
  double InMillisecondsF() const;
  int64_t InMilliseconds() const;
  int64_t InMillisecondsRoundedUp() const;
  constexpr int64_t InMicroseconds() const { return delta_; }
  double InMicrosecondsF() const;
  constexpr int64_t InNanoseconds() const;

  // Computations with other deltas.
  constexpr TimeDelta operator+(TimeDelta other) const;
  constexpr TimeDelta operator-(TimeDelta other) const;

  constexpr TimeDelta& operator+=(TimeDelta other) {
    return *this = (*this + other);
  }
  constexpr TimeDelta& operator-=(TimeDelta other) {
    return *this = (*this - other);
  }
  constexpr TimeDelta operator-() const {
    if (!is_inf())
      return TimeDelta(-delta_);
    return (delta_ < 0) ? Max() : Min();
  }
  // Computations with numeric types.
  template <typename T>
  constexpr TimeDelta operator*(T a) const {
    CheckedNumeric<int64_t> rv(delta_);
    rv *= a;
    if (rv.IsValid())
      return TimeDelta(rv.ValueOrDie());
    return ((delta_ < 0) == (a < 0)) ? Max() : Min();
  }
  template <typename T>
  constexpr TimeDelta operator/(T a) const {
    CheckedNumeric<int64_t> rv(delta_);
    rv /= a;
    if (rv.IsValid())
      return TimeDelta(rv.ValueOrDie());
    return ((delta_ < 0) == (a < 0)) ? Max() : Min();
  }

  template <typename T>
  constexpr TimeDelta& operator*=(T a) {
    return *this = (*this * a);
  }
  template <typename T>
  constexpr TimeDelta& operator/=(T a) {
    return *this = (*this / a);
  }
  // This does floating-point division. For an integer result, either call
  // IntDiv(), or (possibly clearer) use this operator with
  // base::Clamp{Ceil,Floor,Round}() or base::saturated_cast() (for truncation).
  // Note that converting to double here drops precision to 53 bits.
  constexpr double operator/(TimeDelta a) const {
    // 0/0 and inf/inf (any combination of positive and negative) are invalid
    // (they are almost certainly not intentional, and result in NaN, which
    // turns into 0 if clamped to an integer; this makes introducing subtle bugs
    // too easy).
    LTCHECK(!is_zero() || !a.is_zero());
    LTCHECK(!is_inf() || !a.is_inf());
    return ToDouble() / a.ToDouble();
  }

  constexpr int64_t IntDiv(TimeDelta a) const {
    if (!is_inf() && !a.is_zero())
      return delta_ / a.delta_;
    // For consistency, use the same edge case
    // CHECKs and behavior as the code above.
    LTCHECK(!is_zero() || !a.is_zero());
    LTCHECK(!is_inf() || !a.is_inf());
    return ((delta_ < 0) == (a.delta_ < 0))
               ? std::numeric_limits<int64_t>::max()
               : std::numeric_limits<int64_t>::min();
  }

  constexpr TimeDelta operator%(TimeDelta a) const {
#define _TIME_DELTA_VALID (is_inf() || a.is_zero() || a.is_inf())
    return TimeDelta(_TIME_DELTA_VALID ? delta_ : (delta_ % a.delta_));
#undef _TIME_DELTA_VALID
  }
  constexpr TimeDelta& operator%=(TimeDelta other) {
    return *this = (*this % other);
  }

  // Comparison operators.
  constexpr bool operator==(TimeDelta other) const {
    return delta_ == other.delta_;
  }
  constexpr bool operator!=(TimeDelta other) const {
    return delta_ != other.delta_;
  }
  constexpr bool operator<(TimeDelta other) const {
    return delta_ < other.delta_;
  }
  constexpr bool operator<=(TimeDelta other) const {
    return delta_ <= other.delta_;
  }
  constexpr bool operator>(TimeDelta other) const {
    return delta_ > other.delta_;
  }
  constexpr bool operator>=(TimeDelta other) const {
    return delta_ >= other.delta_;
  }

  // Returns this delta, ceiled/floored/rounded-away-from-zero to the nearest
  // multiple of |interval|.
  TimeDelta CeilToMultiple(TimeDelta interval) const;
  TimeDelta FloorToMultiple(TimeDelta interval) const;
  TimeDelta RoundToMultiple(TimeDelta interval) const;

private:
  // Constructs a delta given the duration in microseconds. This is private
  // to avoid confusion by callers with an integer constructor. Use
  // FromSeconds, FromMilliseconds, etc. instead.
  constexpr explicit TimeDelta(int64_t delta_us) : delta_(delta_us) {}

  // Returns a double representation of this TimeDelta's tick count.  In
  // particular, Max()/Min() are converted to +/-infinity.
  constexpr double ToDouble() const {
    if (!is_inf())
      return static_cast<double>(delta_);
    return (delta_ < 0) ? -std::numeric_limits<double>::infinity()
                        : std::numeric_limits<double>::infinity();
  }

  // Delta in microseconds.
  int64_t delta_ = 0;
};

constexpr TimeDelta TimeDelta::operator+(TimeDelta other) const {
  if (!other.is_inf())
    return TimeDelta(int64_t{base::ClampAdd(delta_, other.delta_)});

  // Additions involving two infinities are only valid if signs match.
  LTCHECK(!is_inf() || (delta_ == other.delta_));
  return other;
}

constexpr TimeDelta TimeDelta::operator-(TimeDelta other) const {
  if (!other.is_inf())
    return TimeDelta(int64_t{base::ClampSub(delta_, other.delta_)});

  // Subtractions involving two infinities are only valid if signs differ.
  LTCHECK_NE(delta_, other.delta_);
  return (other.delta_ < 0) ? Max() : Min();
}

template <typename T>
constexpr TimeDelta operator*(T a, TimeDelta td) {
  return td * a;
}

// static
constexpr TimeDelta TimeDelta::FromSeconds(int64_t secs) {
  return TimeDelta(int64_t{base::ClampMul(secs, kMicrosecondsPerSecond)});
}

// static
constexpr TimeDelta TimeDelta::FromSecondsD(double secs) {
  return TimeDelta(saturated_cast<int64_t>(secs * kMicrosecondsPerSecond));
}

// static
constexpr TimeDelta TimeDelta::FromMilliseconds(int64_t ms) {
  return TimeDelta(int64_t{base::ClampMul(ms, kMicrosecondsPerMillisecond)});
}

// static
constexpr TimeDelta TimeDelta::FromMicroseconds(int64_t us) {
  return TimeDelta(us);
}

// static
constexpr TimeDelta TimeDelta::FromNanoseconds(int64_t ns) {
  return TimeDelta(ns / kNanosecondsPerMicrosecond);
}

// static
constexpr TimeDelta TimeDelta::FromHz(double frequency) {
  return FromSeconds(1) / frequency;
}

constexpr int TimeDelta::InHours() const {
  // saturated_cast<> is necessary since very large (but still less than
  // min/max) deltas would result in overflow.
  return saturated_cast<int>(delta_ / kMicrosecondsPerHour);
}

constexpr int TimeDelta::InMinutes() const {
  // saturated_cast<> is necessary since very large (but still less than
  // min/max) deltas would result in overflow.
  return saturated_cast<int>(delta_ / kMicrosecondsPerMinute);
}

constexpr double TimeDelta::InSecondsF() const {
  if (!is_inf())
    return static_cast<double>(delta_) / kMicrosecondsPerSecond;
  return (delta_ < 0) ? -std::numeric_limits<double>::infinity()
                      : std::numeric_limits<double>::infinity();
}

constexpr int64_t TimeDelta::InSeconds() const {
  return is_inf() ? delta_ : (delta_ / kMicrosecondsPerSecond);
}

constexpr int64_t TimeDelta::InNanoseconds() const {
  return base::ClampMul(delta_, kNanosecondsPerMicrosecond);
}

// static
constexpr TimeDelta TimeDelta::Max() {
  return TimeDelta(std::numeric_limits<int64_t>::max());
}

// static
constexpr TimeDelta TimeDelta::Min() {
  return TimeDelta(std::numeric_limits<int64_t>::min());
}

// static
constexpr TimeDelta TimeDelta::FiniteMax() {
  return TimeDelta(std::numeric_limits<int64_t>::max() - 1);
}

// static
constexpr TimeDelta TimeDelta::FiniteMin() {
  return TimeDelta(std::numeric_limits<int64_t>::min() + 1);
}

}  // namespace base
#endif
