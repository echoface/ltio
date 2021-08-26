#ifndef __LT_BASE_STRING_VIEW_H_H
#define __LT_BASE_STRING_VIEW_H_H

// NOTE:
// according to the souce code, c++17 later,
// string_view.hpp will use std::string_view automaticly
#include "string_view.hpp"

namespace std {
  using string_view = nonstd::string_view;
}

namespace base {
  using StringPiece = std::string_view;
}

#endif
