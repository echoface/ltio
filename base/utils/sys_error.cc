#include "config.h"
#include <string.h>
#include "sys_error.h"
#include <vector>

#if HAVE_ERRNO_H
#include <errno.h>
#endif

namespace base {

std::string StrError() {
  return StrError(errno);
}

std::string StrError(int errnum) {

  std::string error_str;
  if (errnum == 0)
    return error_str;

  const int MaxErrStrLen = 256;
  static std::vector<char> buffer(MaxErrStrLen);

  buffer[0] = '\0';

//posix
#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))

#if defined(__GLIBC__)
   // glibc defines its own incompatible version of strerror_r
   // which may not use the buffer supplied.
   error_str = strerror_r(errnum, &buffer[0], MaxErrStrLen - 1);
#else
   strerror_r(errnum, &buffer[0], MaxErrStrLen - 1);
   error_str = (char*)buffer[0];
#endif

#else
  error_str = strerror(errnum);
#endif

  return error_str;
}  // namespace base

}
