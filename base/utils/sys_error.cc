#include "config.h"    
#include <string.h>
#include "sys_error.h"
#include <vector>
 
#if HAVE_ERRNO_H
#include <errno.h>
#endif
 
namespace base {
 
std::string StrErrno() {
  return StrError(errno);
}

std::string StrError(int errnum) {

  std::string str;
  if (errnum == 0)
    return str;

  const int MaxErrStrLen = 2000;
  static std::vector<char> buffer(MaxErrStrLen);  

  buffer[0] = '\0';

#if defined(HAVE_STRERROR_R)
 
#if defined(__GLIBC__)
   // glibc defines its own incompatible version of strerror_r
   // which may not use the buffer supplied.
   str = strerror_r(errnum, &buffer[0], MaxErrStrLen - 1);
#else
   strerror_r(errnum, &buffer[0], MaxErrStrLen - 1);
   str = buffer;
#endif
  
#else
  str = strerror(errnum);
#endif

  return str;
}  // namespace base

}
