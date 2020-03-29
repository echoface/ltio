#ifndef BASE_SYSERRORNO_H_
#define BASE_SYSERRORNO_H_

#include <cerrno>
#include <string>

namespace base {

std::string StrError();
// Like the no-argument version above, but uses errnum instead of errno.
std::string StrError(int errnum);

} // end base
#endif
