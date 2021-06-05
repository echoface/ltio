#include "time.h"

#include <mutex>
#include <sys/time.h>

#include "nspr/prtime.h"
#include "fmt/printf.h"

namespace base {

typedef time_t SysTime;

namespace {
  static std::mutex lock_;

  Time TimeNowIgnoringOverride() {
    struct timeval tv;
    struct timezone tz = {0, 0};  // UTC
    CHECK(gettimeofday(&tv, &tz) == 0);
    // Combine seconds and microseconds in a 64-bit field containing microseconds
    // since the epoch.  That's enough for nearly 600 centuries.  Adjust from
    // Unix (1970) to Windows (1601) epoch.
    return Time() + TimeDelta::FromMicroseconds(
      (tv.tv_sec * TimeDelta::kMicrosecondsPerSecond + tv.tv_usec)
      + Time::kTimeTToMicrosecondsOffset);
  }


SysTime SysTimeFromTimeStruct(struct tm* timestruct, bool is_local) {
  std::lock_guard<std::mutex> guard(lock_);
  return is_local ? mktime(timestruct) : timegm(timestruct);
}

void SysTimeToTimeStruct(SysTime t, struct tm* timestruct, bool is_local) {
  std::lock_guard<std::mutex> guard(lock_);
  if (is_local)
    localtime_r(&t, timestruct);
  else
    gmtime_r(&t, timestruct);
}

} //end namespace

//static
Time Time::UnixEpoch() {
  return Time(kTimeTToMicrosecondsOffset);
}

//static
Time Time::Now() {
  return TimeNowIgnoringOverride();
}

//static
Time Time::NowFromSystemTime() {
  return TimeNowIgnoringOverride();
}

// static
Time Time::FromTimeT(time_t tt) {
  if (tt == 0)
    return Time();  // Preserve 0 so we can tell it doesn't exist.
  return (tt == std::numeric_limits<time_t>::max()) ?
    Max() : (UnixEpoch() + TimeDelta::FromSeconds(tt));
}

time_t Time::ToTimeT() const {
  if (is_null())
    return 0;  // Preserve 0 so we can tell it doesn't exist.
  if (!is_inf() && ((std::numeric_limits<int64_t>::max() -
                     kTimeTToMicrosecondsOffset) > us_))
    return (*this - UnixEpoch()).InSeconds();
  return (us_ < 0) ? std::numeric_limits<time_t>::min()
    : std::numeric_limits<time_t>::max();
}


// static
Time Time::FromDoubleT(double dt) {
  // Preserve 0 so we can tell it doesn't exist.
  return (dt == 0 || std::isnan(dt))
    ? Time() : (UnixEpoch() + TimeDelta::FromSecondsD(dt));
}

double Time::ToDoubleT() const {
  if (is_null())
    return 0;  // Preserve 0 so we can tell it doesn't exist.
  if (!is_inf())
    return (*this - UnixEpoch()).InSecondsF();
  return (us_ < 0) ? -std::numeric_limits<double>::infinity()
                   : std::numeric_limits<double>::infinity();
}

// static
Time Time::FromTimeSpec(const timespec& ts) {
  return FromDoubleT(ts.tv_sec + double(ts.tv_nsec) / TimeDelta::kNanosecondsPerSecond);
}

// static
Time Time::FromTimeVal(struct timeval t) {
  CHECK(t.tv_usec > 0);
  CHECK(t.tv_usec < static_cast<int>(TimeDelta::kMicrosecondsPerSecond));

  if (t.tv_usec == 0 && t.tv_sec == 0) {
    return Time();
  }

  if (t.tv_usec == static_cast<suseconds_t>(TimeDelta::kMicrosecondsPerSecond) - 1 &&
      t.tv_sec == std::numeric_limits<time_t>::max()) {
    return Max();
  }
  return Time((static_cast<int64_t>(t.tv_sec) * TimeDelta::kMicrosecondsPerSecond) +
              t.tv_usec + kTimeTToMicrosecondsOffset);
}

struct timeval Time::ToTimeVal() const {
  struct timeval result;
  if (is_null()) {
    result.tv_sec = 0;
    result.tv_usec = 0;
    return result;
  }
  if (is_max()) {
    result.tv_sec = std::numeric_limits<time_t>::max();
    result.tv_usec = static_cast<suseconds_t>(TimeDelta::kMicrosecondsPerSecond) - 1;
    return result;
  }
  int64_t us = us_ - kTimeTToMicrosecondsOffset;
  result.tv_sec = us / TimeDelta::kMicrosecondsPerSecond;
  result.tv_usec = us % TimeDelta::kMicrosecondsPerSecond;
  return result;
}


Time Time::Midnight(bool is_local) const {
  Exploded exploded;
  Explode(is_local, &exploded);
  exploded.hour = 0;
  exploded.minute = 0;
  exploded.second = 0;
  exploded.millisecond = 0;
  Time out_time;
  if (FromExploded(is_local, exploded, &out_time))
    return out_time;

  // Reaching here means 00:00:00am of the current day does not exist (due to
  // Daylight Saving Time in some countries where clocks are shifted at
  // midnight). In this case, midnight should be defined as 01:00:00am.
  CHECK(is_local);
  exploded.hour = 1;
  const bool result = FromExploded(is_local, exploded, &out_time);
  CHECK(result);  // This function must not fail.
  return out_time;
}

void Time::Explode(bool is_local, Exploded* exploded) const {
  const int64_t millis_since_unix_epoch =
      ToRoundedDownMillisecondsSinceUnixEpoch();

  // Split the |millis_since_unix_epoch| into separate seconds and millisecond
  // components because the platform calendar-explode operates at one-second
  // granularity.
  SysTime seconds = millis_since_unix_epoch / TimeDelta::kMillisecondsPerSecond;
  int64_t millisecond = millis_since_unix_epoch % TimeDelta::kMillisecondsPerSecond;
  if (millisecond < 0) {
    // Make the the |millisecond| component positive, within the range [0,999],
    // by transferring 1000 ms from |seconds|.
    --seconds;
    millisecond += TimeDelta::kMillisecondsPerSecond;
  }

  struct tm timestruct;
  SysTimeToTimeStruct(seconds, &timestruct, is_local);

  exploded->year = timestruct.tm_year + 1900;
  exploded->month = timestruct.tm_mon + 1;
  exploded->day_of_week = timestruct.tm_wday;
  exploded->day_of_month = timestruct.tm_mday;
  exploded->hour = timestruct.tm_hour;
  exploded->minute = timestruct.tm_min;
  exploded->second = timestruct.tm_sec;
  exploded->millisecond = millisecond;
}

// static
bool Time::FromExploded(bool is_local, const Exploded& exploded, Time* time) {
  CheckedNumeric<int> month = exploded.month;
  month--;
  CheckedNumeric<int> year = exploded.year;
  year -= 1900;
  if (!month.IsValid() || !year.IsValid()) {
    *time = Time(0);
    return false;
  }

  struct tm timestruct;
  timestruct.tm_sec = exploded.second;
  timestruct.tm_min = exploded.minute;
  timestruct.tm_hour = exploded.hour;
  timestruct.tm_mday = exploded.day_of_month;
  timestruct.tm_mon = month.ValueOrDie();
  timestruct.tm_year = year.ValueOrDie();
  timestruct.tm_wday = exploded.day_of_week;  // mktime/timegm ignore this
  timestruct.tm_yday = 0;                     // mktime/timegm ignore this
  timestruct.tm_isdst = -1;                   // attempt to figure it out
#if !defined(OS_NACL) && !defined(OS_SOLARIS) && !defined(OS_AIX)
  timestruct.tm_gmtoff = 0;   // not a POSIX field, so mktime/timegm ignore
  timestruct.tm_zone = nullptr;  // not a POSIX field, so mktime/timegm ignore
#endif

  SysTime seconds;

  // Certain exploded dates do not really exist due to daylight saving times,
  // and this causes mktime() to return implementation-defined values when
  // tm_isdst is set to -1. On Android, the function will return -1, while the
  // C libraries of other platforms typically return a liberally-chosen value.
  // Handling this requires the special code below.

  // SysTimeFromTimeStruct() modifies the input structure, save current value.
  struct tm timestruct0 = timestruct;

  seconds = SysTimeFromTimeStruct(&timestruct, is_local);
  if (seconds == -1) {
    // Get the time values with tm_isdst == 0 and 1, then select the closest one
    // to UTC 00:00:00 that isn't -1.
    timestruct = timestruct0;
    timestruct.tm_isdst = 0;
    int64_t seconds_isdst0 = SysTimeFromTimeStruct(&timestruct, is_local);

    timestruct = timestruct0;
    timestruct.tm_isdst = 1;
    int64_t seconds_isdst1 = SysTimeFromTimeStruct(&timestruct, is_local);

    // seconds_isdst0 or seconds_isdst1 can be -1 for some timezones.
    // E.g. "CLST" (Chile Summer Time) returns -1 for 'tm_isdt == 1'.
    if (seconds_isdst0 < 0)
      seconds = seconds_isdst1;
    else if (seconds_isdst1 < 0)
      seconds = seconds_isdst0;
    else
      seconds = std::min(seconds_isdst0, seconds_isdst1);
  }

  // Handle overflow.  Clamping the range to what mktime and timegm might
  // return is the best that can be done here.  It's not ideal, but it's better
  // than failing here or ignoring the overflow case and treating each time
  // overflow as one second prior to the epoch.
  int64_t milliseconds = 0;
  if (seconds == -1 && (exploded.year < 1969 || exploded.year > 1970)) {
    // If exploded.year is 1969 or 1970, take -1 as correct, with the
    // time indicating 1 second prior to the epoch.  (1970 is allowed to handle
    // time zone and DST offsets.)  Otherwise, return the most future or past
    // time representable.  Assumes the time_t epoch is 1970-01-01 00:00:00 UTC.
    //
    // The minimum and maximum representible times that mktime and timegm could
    // return are used here instead of values outside that range to allow for
    // proper round-tripping between exploded and counter-type time
    // representations in the presence of possible truncation to time_t by
    // division and use with other functions that accept time_t.
    //
    // When representing the most distant time in the future, add in an extra
    // 999ms to avoid the time being less than any other possible value that
    // this function can return.

    // On Android, SysTime is int64_t, special care must be taken to avoid
    // overflows.
    const int64_t min_seconds = (sizeof(SysTime) < sizeof(int64_t))
                                    ? std::numeric_limits<SysTime>::min()
                                    : std::numeric_limits<int32_t>::min();
    const int64_t max_seconds = (sizeof(SysTime) < sizeof(int64_t))
                                    ? std::numeric_limits<SysTime>::max()
                                    : std::numeric_limits<int32_t>::max();
    if (exploded.year < 1969) {
      milliseconds = min_seconds * TimeDelta::kMillisecondsPerSecond;
    } else {
      milliseconds = max_seconds * TimeDelta::kMillisecondsPerSecond;
      milliseconds += (TimeDelta::kMillisecondsPerSecond - 1);
    }
  } else {
    CheckedNumeric<int64_t> checked_millis = seconds;
    checked_millis *= TimeDelta::kMillisecondsPerSecond;
    checked_millis += exploded.millisecond;
    if (!checked_millis.IsValid()) {
      *time = Time(0);
      return false;
    }
    milliseconds = checked_millis.ValueOrDie();
  }

  Time converted_time;
  if (!FromMillisecondsSinceUnixEpoch(milliseconds, &converted_time)) {
    *time = base::Time(0);
    return false;
  }

  // If |exploded.day_of_month| is set to 31 on a 28-30 day month, it will
  // return the first day of the next month. Thus round-trip the time and
  // compare the initial |exploded| with |utc_to_exploded| time.
  Time::Exploded to_exploded;
  if (!is_local)
    converted_time.UTCExplode(&to_exploded);
  else
    converted_time.LocalExplode(&to_exploded);

  if (ExplodedMostlyEquals(to_exploded, exploded)) {
    *time = converted_time;
    return true;
  }

  *time = Time(0);
  return false;
}

// static
bool Time::FromStringInternal(const char* time_string,
                              bool is_local,
                              Time* parsed_time) {
  CHECK(time_string);
  CHECK(parsed_time);

  if (time_string[0] == '\0')
    return false;

  PRTime result_time = 0;
  PRStatus result = PR_ParseTimeString(time_string,
                                       is_local ? PR_FALSE : PR_TRUE,
                                       &result_time);
  if (result != PR_SUCCESS)
    return false;

  *parsed_time = UnixEpoch() + TimeDelta::FromMicroseconds(result_time);
  return true;
}

// static
bool Time::ExplodedMostlyEquals(const Exploded& lhs, const Exploded& rhs) {
  return std::tie(lhs.year, lhs.month, lhs.day_of_month, lhs.hour, lhs.minute,
                  lhs.second, lhs.millisecond) ==
    std::tie(rhs.year, rhs.month, rhs.day_of_month, rhs.hour, rhs.minute,
             rhs.second, rhs.millisecond);
}


// static
bool Time::FromMillisecondsSinceUnixEpoch(int64_t unix_milliseconds,
                                          Time* time) {
  // Adjust the provided time from milliseconds since the Unix epoch (1970) to
  // microseconds since the Windows epoch (1601), avoiding overflows.
  CheckedNumeric<int64_t> checked_microseconds_win_epoch = unix_milliseconds;
  checked_microseconds_win_epoch *= TimeDelta::kMicrosecondsPerMillisecond;
  checked_microseconds_win_epoch += kTimeTToMicrosecondsOffset;
  *time = Time(checked_microseconds_win_epoch.ValueOrDefault(0));
  return checked_microseconds_win_epoch.IsValid();
}


int64_t Time::ToRoundedDownMillisecondsSinceUnixEpoch() const {
  constexpr int64_t kEpochOffsetMillis =
      kTimeTToMicrosecondsOffset / TimeDelta::kMicrosecondsPerMillisecond;
  static_assert(kTimeTToMicrosecondsOffset % TimeDelta::kMicrosecondsPerMillisecond == 0,
                "assumption: no epoch offset sub-milliseconds");

  // Compute the milliseconds since UNIX epoch without the possibility of
  // under/overflow. Round the result towards -infinity.
  //
  // If |us_| is negative and includes fractions of a millisecond, subtract one
  // more to effect the round towards -infinity. C-style integer truncation
  // takes care of all other cases.
  const int64_t millis = us_ / TimeDelta::kMicrosecondsPerMillisecond;
  const int64_t submillis = us_ % TimeDelta::kMicrosecondsPerMillisecond;
  return millis - kEpochOffsetMillis - (submillis < 0);
}

std::ostream& operator<<(std::ostream& os, Time time) {
  Time::Exploded exploded;
  time.UTCExplode(&exploded);
  // Use StringPrintf because iostreams formatting is painful.
  return os << fmt::sprintf("%04d-%02d-%02d %02d:%02d:%02d.%03d UTC",
                            exploded.year,
                            exploded.month,
                            exploded.day_of_month,
                            exploded.hour,
                            exploded.minute,
                            exploded.second,
                            exploded.millisecond);
}


} //end base
