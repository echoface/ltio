#include "str_utils.h"

namespace base {

bool StrUtils::Replace(std::string &str, const std::string &from, const std::string &to) {
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos) return false;
  str.replace(start_pos, from.length(), to);
  return true;
}

bool StrUtils::ReplaceLast(std::string &str, const std::string &from, const std::string &to) {
  size_t start_pos = str.rfind(from);
  if (start_pos == std::string::npos) return false;
  str.replace(start_pos, from.length(), to);
  return true;
}

bool StrUtils::Contains(const std::string& input, const std::string& test) {
  return std::search(input.begin(),
                     input.end(),
                     test.begin(),
                     test.end()) != input.end();
}

bool StrUtils::StartsWith(const std::string &str, const std::string &prefix) {
  return str.size() >= prefix.size() &&
    str.compare(0, prefix.size(), prefix) == 0;
}

bool StrUtils::EndsWith(const std::string &str, const std::string &suffix) {
  return str.size() >= suffix.size() &&
    str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string& StrUtils::TrimLeft(std::string &s) {
  s.erase(s.begin(),
          std::find_if(s.begin(), s.end(),
                       std::not1(std::ptr_fun<int, int>(std::isspace))));
  return s;
}

std::string& StrUtils::TrimRight(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       std::not1(std::ptr_fun<int, int>(std::isspace)))
          .base(),
          s.end());
  return s;
}
std::vector<std::string> StrUtils::Split(const std::string &str, const char delim) {

  std::vector<std::string> elems;
  std::stringstream ss(str);

  std::string item;
  while (getline(ss, item, delim)) elems.push_back(item);
  return std::move(elems);
}

std::vector<std::string> StrUtils::Split(const std::string &text,
                                         const std::string &delims,
                                         bool ignore_empty = true) {

  std::vector<std::string> tokens;
  size_t prev = 0, pos = 0;
  do {
    pos = text.find(delims, prev);
    if (pos == std::string::npos) pos = text.length();
    std::string token = text.substr(prev, pos-prev);
    if (!token.empty() || !ignore_empty) tokens.push_back(token);
    prev = pos + delims.length();
  } while (pos < text.length() && prev < text.length());
  return std::move(tokens);
}


} // enabase
