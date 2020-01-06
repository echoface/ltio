#include "str_utils.h"

namespace base {

void StrUtil::ToUpper(char *s) {
    for (size_t i = 0; s[i]; i++) s[i] = toupper(s[i]);
}
void StrUtil::ToUpper(std::string &str) {
  for (size_t i = 0; str[i]; i++) str[i] = toupper(str[i]);
}
void StrUtil::ToLower(char *s) {
  for (size_t i = 0; s[i]; i++) s[i] = tolower(s[i]);
}
void StrUtil::ToLower(std::string &str) {
  for (size_t i = 0; str[i]; i++) str[i] = tolower(str[i]);
}

bool StrUtil::Replace(std::string &str, const std::string &from, const std::string &to) {
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos) return false;
  str.replace(start_pos, from.length(), to);
  return true;
}

bool StrUtil::ReplaceLast(std::string &str, const std::string &from, const std::string &to) {
  size_t start_pos = str.rfind(from);
  if (start_pos == std::string::npos) return false;
  str.replace(start_pos, from.length(), to);
  return true;
}

bool StrUtil::Contains(const std::string& input, const std::string& test) {
  return std::search(input.begin(),
                     input.end(),
                     test.begin(),
                     test.end()) != input.end();
}

bool StrUtil::StartsWith(const std::string &str, const std::string &prefix) {
  return str.size() >= prefix.size() &&
    str.compare(0, prefix.size(), prefix) == 0;
}

bool StrUtil::EndsWith(const std::string &str, const std::string &suffix) {
  return str.size() >= suffix.size() &&
    str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void StrUtil::TrimLeft(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
    return !std::isspace(ch);
  }));
}

void StrUtil::TrimRight(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
    return !std::isspace(ch);
  }).base(), s.end());
}

std::vector<std::string> StrUtil::Split(const std::string &str,
                                        const char delim) {
  std::vector<std::string> elems;
  std::string token;
  std::stringstream ss(str);
  while (std::getline(ss, token, delim)) {
    elems.push_back(token);
  }
  return std::move(elems);
}

std::vector<std::string> StrUtil::Split(const std::string &text,
                                        const std::string &delims,
                                        bool ignore_empty) {
  std::vector<std::string> tokens;
  std::size_t current, previous = 0;
  current = text.find(delims);
  while (current != std::string::npos) {
    if (current > previous) {
      tokens.push_back(text.substr(previous, current - previous));
    } else if (!ignore_empty) {
      tokens.push_back(text.substr(previous, current - previous));
    }
    previous = current + 1;
    current = text.find(delims, previous);
  }
  if (current > previous) {
    tokens.push_back(text.substr(previous, current - previous));
  } else if (!ignore_empty) {
    tokens.push_back(text.substr(previous, current - previous));
  }
  return tokens;
}

} // ena base
