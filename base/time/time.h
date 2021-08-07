#ifndef _LT_BASE_TIME_H_H
#define _LT_BASE_TIME_H_H

#include <limits>
#include <ostream>

#include <base/lt_micro.h>
#include <base/compiler_specific.h>
#include <base/time/time_base.h>

namespace base {

// Time -----------------------------------------------------------------------

// Represents a wall clock time in UTC. Values are not guaranteed to be
// monotonically non-decreasing and are subject to large amounts of skew.
// Time is stored internally as microseconds since the Windows epoch (1601).
class Time : public time_internal::TimeBase<Time> {
public:
  // Offset of UNIX epoch (1970-01-01 00:00:00 UTC) from Windows FILETIME epoch
  // (1601-01-01 00:00:00 UTC), in microseconds. This value is derived from the
  // following: ((1970-1601)*365+89)*24*60*60*1000*1000, where 89 is the number
  // of leap year days between 1601 and 1970: (1970-1601)/4 excluding 1700,
  // 1800, and 1900.
  static constexpr int64_t kTimeTToMicrosecondsOffset =
      INT64_C(11644473600000000);

#if defined(OS_WIN)
  // To avoid overflow in QPC to Microseconds calculations, since we multiply
  // by kMicrosecondsPerSecond, then the QPC value should not exceed
  // (2^63 - 1) / 1E6. If it exceeds that threshold, we divide then multiply.
  static constexpr int64_t kQPCOverflowThreshold = INT64_C(0x8637BD05AF7);
#endif

// kExplodedMinYear and kExplodedMaxYear define the platform-specific limits
// for values passed to FromUTCExploded() and FromLocalExploded(). Those
// functions will return false if passed values outside these limits. The limits
// are inclusive, meaning that the API should support all dates within a given
// limit year.
//
// WARNING: These are not the same limits for the inverse functionality,
// UTCExplode() and LocalExplode(). See method comments for further details.
#if defined(OS_WIN)
  static constexpr int kExplodedMinYear = 1601;
  static constexpr int kExplodedMaxYear = 30827;
#elif defined(OS_IOS) && !__LP64__
  static constexpr int kExplodedMinYear = std::numeric_limits<int>::min();
  static constexpr int kExplodedMaxYear = std::numeric_limits<int>::max();
#elif defined(OS_APPLE)
  static constexpr int kExplodedMinYear = 1902;
  static constexpr int kExplodedMaxYear = std::numeric_limits<int>::max();
#elif defined(OS_ANDROID)
  // Though we use 64-bit time APIs on both 32 and 64 bit Android, some OS
  // versions like KitKat (ARM but not x86 emulator) can't handle some early
  // dates (e.g. before 1170). So we set min conservatively here.
  static constexpr int kExplodedMinYear = 1902;
  static constexpr int kExplodedMaxYear = std::numeric_limits<int>::max();
#else
  static constexpr int kExplodedMinYear =
      (sizeof(time_t) == 4 ? 1902 : std::numeric_limits<int>::min());
  static constexpr int kExplodedMaxYear =
      (sizeof(time_t) == 4 ? 2037 : std::numeric_limits<int>::max());
#endif

  // Represents an exploded time that can be formatted nicely. This is kind of
  // like the Win32 SYSTEMTIME structure or the Unix "struct tm" with a few
  // additions and changes to prevent errors.
  // This structure always represents dates in the Gregorian calendar and always
  // encodes day_of_week as Sunday==0, Monday==1, .., Saturday==6. This means
  // that base::Time::LocalExplode and base::Time::FromLocalExploded only
  // respect the current local time zone in the conversion and do *not* use a
  // calendar or day-of-week encoding from the current locale.
  struct Exploded {
    int year;          // Four digit year "2007"
    int month;         // 1-based month (values 1 = January, etc.)
    int day_of_week;   // 0-based day of week (0 = Sunday, etc.)
    int day_of_month;  // 1-based day of month (1-31)
    int hour;          // Hour within the current day (0-23)
    int minute;        // Minute within the current hour (0-59)
    int second;        // Second within the current minute (0-59 plus leap
                       //   seconds which may take it up to 60).
    int millisecond;   // Milliseconds within the current second (0-999)

    // A cursory test for whether the data members are within their
    // respective ranges. A 'true' return value does not guarantee the
    // Exploded value can be successfully converted to a Time value.
    bool HasValidValues() const;
  };

  // Contains the NULL time. Use Time::Now() to get the current time.
  constexpr Time() : TimeBase(0) {}

  // Returns the time for epoch in Unix-like system (Jan 1, 1970).
  static Time UnixEpoch();

  // Returns the current time. Watch out, the system might adjust its clock
  // in which case time will actually go backwards. We don't guarantee that
  // times are increasing, or that two calls to Now() won't be the same.
  static Time Now();

  // Returns the current time. Same as Now() except that this function always
  // uses system time so that there are no discrepancies between the returned
  // time and system time even on virtual environments including our test bot.
  // For timing sensitive unittests, this function should be used.
  static Time NowFromSystemTime();

  // Converts to/from time_t in UTC and a Time class.
  static Time FromTimeT(time_t tt);
  time_t ToTimeT() const;

  // Converts time to/from a double which is the number of seconds since epoch
  // (Jan 1, 1970).  Webkit uses this format to represent time.
  // Because WebKit initializes double time value to 0 to indicate "not
  // initialized", we map it to empty Time object that also means "not
  // initialized".
  static Time FromDoubleT(double dt);
  double ToDoubleT() const;

  // Converts the timespec structure to time. MacOS X 10.8.3 (and tentatively,
  // earlier versions) will have the |ts|'s tv_nsec component zeroed out,
  // having a 1 second resolution, which agrees with
  // https://developer.apple.com/legacy/library/#technotes/tn/tn1150.html#HFSPlusDates.
  static Time FromTimeSpec(const timespec& ts);

  static Time FromTimeVal(struct timeval t);
  struct timeval ToTimeVal() const;

  // Converts an exploded structure representing either the local time or UTC
  // into a Time class. Returns false on a failure when, for example, a day of
  // month is set to 31 on a 28-30 day month. Returns Time(0) on overflow.
  // FromLocalExploded respects the current time zone but does not attempt to
  // use the calendar or day-of-week encoding from the current locale - see the
  // comments on base::Time::Exploded for more information.
  static bool FromUTCExploded(const Exploded& exploded,
                              Time* time) WARN_UNUSED_RESULT {
    return FromExploded(false, exploded, time);
  }
  static bool FromLocalExploded(const Exploded& exploded,
                                Time* time) WARN_UNUSED_RESULT {
    return FromExploded(true, exploded, time);
  }

  // Converts a string representation of time to a Time object.
  // An example of a time string which is converted is as below:-
  // "Tue, 15 Nov 1994 12:45:26 GMT". If the timezone is not specified
  // in the input string, FromString assumes local time and FromUTCString
  // assumes UTC. A timezone that cannot be parsed (e.g. "UTC" which is not
  // specified in RFC822) is treated as if the timezone is not specified.
  //
  // WARNING: the underlying converter is very permissive. For example: it is
  // not checked whether a given day of the week matches the date; Feb 29
  // silently becomes Mar 1 in non-leap years; under certain conditions, whole
  // English sentences may be parsed successfully and yield unexpected results.
  //
  // TODO(iyengar) Move the FromString/FromTimeT/ToTimeT/FromFileTime to
  // a new time converter class.
  static bool FromString(const char* time_string,
                         Time* parsed_time) WARN_UNUSED_RESULT {
    return FromStringInternal(time_string, true, parsed_time);
  }
  static bool FromUTCString(const char* time_string,
                            Time* parsed_time) WARN_UNUSED_RESULT {
    return FromStringInternal(time_string, false, parsed_time);
  }

  // Fills the given |exploded| structure with either the local time or UTC from
  // this Time instance. If the conversion cannot be made, the output will be
  // assigned invalid values. Use Exploded::HasValidValues() to confirm a
  // successful conversion.
  //
  // Y10K compliance: This method will successfully convert all Times that
  // represent dates on/after the start of the year 1601 and on/before the start
  // of the year 30828. Some platforms might convert over a wider input range.
  // LocalExplode respects the current time zone but does not attempt to use the
  // calendar or day-of-week encoding from the current locale - see the comments
  // on base::Time::Exploded for more information.
  void UTCExplode(Exploded* exploded) const { Explode(false, exploded); }
  void LocalExplode(Exploded* exploded) const { Explode(true, exploded); }

  // The following two functions round down the time to the nearest day in
  // either UTC or local time. It will represent midnight on that day.
  Time UTCMidnight() const { return Midnight(false); }
  Time LocalMidnight() const { return Midnight(true); }

  // For legacy deserialization only. Converts an integer value representing
  // Time to a class. This may be used when deserializing a |Time| structure,
  // using a value known to be compatible. It is not provided as a constructor
  // because the integer type may be unclear from the perspective of a caller.
  //
  // DEPRECATED - Do not use in new code. When deserializing from `base::Value`,
  // prefer the helpers from //base/util/values/values_util.h instead.
  // Otherwise, use `Time::FromDeltaSinceWindowsEpoch()` for `Time` and
  // `TimeDelta::FromMiseconds()` for `TimeDelta`. http://crbug.com/634507
  static constexpr Time FromInternalValue(int64_t us) { return Time(us); }

private:
  friend class time_internal::TimeBase<Time>;

  constexpr explicit Time(int64_t microseconds_since_win_epoch)
    : TimeBase(microseconds_since_win_epoch) {}

  // Explodes the given time to either local time |is_local = true| or UTC
  // |is_local = false|.
  void Explode(bool is_local, Exploded* exploded) const;

  // Unexplodes a given time assuming the source is either local time
  // |is_local = true| or UTC |is_local = false|. Function returns false on
  // failure and sets |time| to Time(0). Otherwise returns true and sets |time|
  // to non-exploded time.
  static bool FromExploded(bool is_local,
                           const Exploded& exploded,
                           Time* time) WARN_UNUSED_RESULT;

  // Rounds down the time to the nearest day in either local time
  // |is_local = true| or UTC |is_local = false|.
  Time Midnight(bool is_local) const;

  // Converts a string representation of time to a Time object.
  // An example of a time string which is converted is as below:-
  // "Tue, 15 Nov 1994 12:45:26 GMT". If the timezone is not specified
  // in the input string, local time |is_local = true| or
  // UTC |is_local = false| is assumed. A timezone that cannot be parsed
  // (e.g. "UTC" which is not specified in RFC822) is treated as if the
  // timezone is not specified.
  static bool FromStringInternal(const char* time_string,
                                 bool is_local,
                                 Time* parsed_time) WARN_UNUSED_RESULT;

  // Comparison does not consider |day_of_week| when doing the operation.
  static bool ExplodedMostlyEquals(const Exploded& lhs,
                                   const Exploded& rhs) WARN_UNUSED_RESULT;

  // Converts the provided time in milliseconds since the Unix epoch (1970) to a
  // Time object, avoiding overflows.
  static bool FromMillisecondsSinceUnixEpoch(int64_t unix_milliseconds,
                                             Time* time) WARN_UNUSED_RESULT;

  // Returns the milliseconds since the Unix epoch (1970), rounding the
  // microseconds towards -infinity.
  int64_t ToRoundedDownMillisecondsSinceUnixEpoch() const;
};

std::ostream& operator<<(std::ostream& os, Time time);

}  // namespace base

#endif
