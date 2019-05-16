#ifndef LIGHTING_BASE_UTILS_Str_H
#define LIGHTING_BASE_UTILS_Str_H

#include <algorithm>
#include <cctype>
#include <cstring>
#include <functional>
#include <locale>
#include <sstream>
#include <string>
#include <vector>
#include "str_detail.hpp"

namespace base {

class Str {
  public:
    static bool Replace(std::string &str, const std::string &from, const std::string &to);
    static bool ReplaceLast(std::string &str, const std::string &from, const std::string &to);

    static bool Contains(const std::string& input, const std::string& test);
    static bool EndsWith(const std::string &str, const std::string &suffix);
    static bool StartsWith(const std::string &str, const std::string &prefix);

    static void ToUpper(char *s) {
      for (size_t i = 0; s[i]; i++) s[i] = toupper(s[i]);
    }
    static void ToUpper(std::string &str) {
      for (size_t i = 0; str[i]; i++) str[i] = toupper(str[i]);
    }
    static void ToLower(char *s) {
      for (size_t i = 0; s[i]; i++) s[i] = tolower(s[i]);
    }
    static void ToLower(std::string &str) {
      for (size_t i = 0; str[i]; i++) str[i] = tolower(str[i]);
    }

    static std::string& TrimLeft(std::string &s);
    static std::string& TrimRight(std::string &s);
    static std::string& Trim(std::string &s) {return TrimLeft(TrimRight(s));}
    static std::vector<std::string> Split(const std::string &str,
                                          const char delim);

    static std::vector<std::string> Split(const std::string &text,
                                          const std::string &delims,
                                          bool ignore_empty = false);


    template <typename T>
      static std::string Join(const std::vector<T> &v, const std::string &token) {
        std::ostringstream result;
        for (auto i = v.begin(); i != v.end(); i++) {
          if (i != v.begin()) {
            result << token;
          }
          result << *i;
        }
        return result.str();
      }

    template <typename T>
      inline static std::vector<T> ParseArray(const std::string &str, const char delim) {
        std::vector<T> elems;
        std::istringstream f(str);
        std::string s;
        while (std::getline(f, s, delim)) {
          elems.push_back(Parse<T>(s));
        }
        return elems;
      }
    template <typename T>
      static std::string ToString(T value) {
        std::ostringstream ss;
        ss << value;
        return ss.str();
      }

    template<typename T>
      static bool ParseTo(const std::string& str, T& out) {
        std::istringstream iss = std::istringstream(str);
        iss >> out;
        return iss.good();
      }

    template <typename T>
      static T Parse(const std::string &str) {
        T result;
        std::istringstream(str) >> result;
        return result;
      }


    template<typename ...Args>
      static std::string Concat(Args&&... args) {
        std::ostringstream oss;
        _str_detail::concat_impl(oss, std::forward<Args>(args)...);
        return oss.str();
      }
};

template <>
inline std::string Str::Parse(const std::string &str) {
  return str;
}

template <>
inline const char* Str::Parse(const std::string &str) {
  return str.c_str();
}

template <>
inline bool Str::Parse(const std::string &str) {
  std::string s = str;
  ToLower(s);

  return s == "1" || s == "on" || s == "yes" || s == "true";
}

}  // namespace base

#endif
