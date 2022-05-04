
#include "number_parser.h"

#include "base/utils/string/str_utils.h"
#include "fmt/format.h"

namespace component {

TokenPtr NumberParser::ParseIndexing(const AttrValues::ValueContainer& values) {
  return ParseQueryAssign(values);
};

TokenPtr NumberParser::ParseQueryAssign(const AttrValues::ValueContainer& vs) const {
  TokenI64* results = new TokenI64();
  int64_t value;
  for (auto& v : vs) {
    if (v.empty()) {
      continue;
    }
    // C++17 if (bool ok = base::StrUtil::ToInt64(v, &value); !ok) {
    bool ok = base::StrUtil::ToInt64(v, &value);
    if (!ok) {
      results->SetFail(fmt::format("{} not valid string", v));
      return TokenPtr(results);
    }
    results->values_.push_back(value);
  }
  return TokenPtr(results);
}


TokenPtr HasherParser::ParseIndexing(const AttrValues::ValueContainer& vs) {
  return ParseQueryAssign(vs);
}

TokenPtr HasherParser::ParseQueryAssign(const AttrValues::ValueContainer& vs) const {
  TokenI64* results = new TokenI64();
  int64_t value;
  for (auto& v : vs) {
    if (v.empty()) {
      continue;
    }
    value = hasher_(v);
    results->values_.push_back(value);
  }
  return TokenPtr(results);
}

}  // namespace component
