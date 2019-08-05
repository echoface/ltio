#include "str_utils.h"

namespace base {

bool Str::Replace(std::string &str, const std::string &from, const std::string &to) {
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos) return false;
  str.replace(start_pos, from.length(), to);
  return true;
}

bool Str::ReplaceLast(std::string &str, const std::string &from, const std::string &to) {
  size_t start_pos = str.rfind(from);
  if (start_pos == std::string::npos) return false;
  str.replace(start_pos, from.length(), to);
  return true;
}

bool Str::Contains(const std::string& input, const std::string& test) {
  return std::search(input.begin(),
                     input.end(),
                     test.begin(),
                     test.end()) != input.end();
}

bool Str::StartsWith(const std::string &str, const std::string &prefix) {
  return str.size() >= prefix.size() &&
    str.compare(0, prefix.size(), prefix) == 0;
}

bool Str::EndsWith(const std::string &str, const std::string &suffix) {
  return str.size() >= suffix.size() &&
    str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string& Str::TrimLeft(std::string &s) {
  auto iter = std::find_if(s.begin(),
                           s.end(),
                           std::not1(std::ptr_fun<int, int>(std::isspace)));
  s.erase(s.begin(), iter);
  return s;
}

std::string& Str::TrimRight(std::string &s) {
  auto v = std::find_if(s.rbegin(),
                        s.rend(),
                        std::not1(std::ptr_fun<int, int>(std::isspace)));
  s.erase(v.base(), s.end());
  return s;
}

std::vector<std::string> Str::Split(const std::string &str,
                                    const char delim) {
  std::vector<std::string> elems;

  std::string token;
  std::stringstream ss(str);
  while (std::getline(ss, token, delim)) {
    elems.push_back(token);
  }
  return std::move(elems);
}

std::vector<std::string> Str::Split(const std::string &text,
                                    const std::string &delims,
                                    bool ignore_empty) {
  std::vector<std::string> tokens;
  std::size_t current, previous = 0;
  current = text.find(delims);
  while (current != std::string::npos) {
    if (current == previous && !ignore_empty) {
      tokens.push_back(text.substr(previous, current - previous));
    } else {
      tokens.push_back(text.substr(previous, current - previous));
    }
    previous = current + 1;
    current = text.find(delims, previous);
  }
  if (current == previous && !ignore_empty) {
    tokens.push_back(text.substr(previous, current - previous));
  } else {
    tokens.push_back(text.substr(previous, current - previous));
  }
  return std::move(tokens);
}

} // enabase
