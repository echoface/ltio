#ifndef LIGHTING_BASE_StrUtils_DETAIL_H
#define LIGHTING_BASE_StrUtils_DETAIL_H

#include <algorithm>
#include <cctype>
#include <cstring>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

namespace base {

class _str_detail {
public:

  static void concat_impl(std::ostream&) { /* do nothing */ }

  template<typename T, typename ...Args>
  static void concat_impl(std::ostream& os, const T& t, Args&&... args) {
     os << t;
     concat_impl(os, std::forward<Args>(args)...);
  }
};

} //

#endif
