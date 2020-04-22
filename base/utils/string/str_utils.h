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

namespace base {

class StrUtil {
    class __detail__ {
      public:
        struct CaseInsensitiveCmp {
          bool operator()(int c1, int c2) const {
            return std::toupper(c1) == std::toupper(c2);
          }
        };

        static void ConcatImpl(std::ostream&) { /* do nothing */ }
        template<typename T, typename ...Args>
          static void ConcatImpl(std::ostream& os, const T& t, Args&&... args) {
            os << t;
            ConcatImpl(os, std::forward<Args>(args)...);
          }

    };

  public:
    static void ToUpper(char *s);
    static void ToUpper(std::string &s);
    static void ToLower(char *s);
    static void ToLower(std::string &s);

    static bool Replace(std::string &s, const std::string &from, const std::string &to);
    static bool ReplaceLast(std::string &s, const std::string &from, const std::string &to);

    static bool Contains(const std::string& input, const std::string& test);
    static bool EndsWith(const std::string &s, const std::string &suffix);
    static bool StartsWith(const std::string &s, const std::string &prefix);

    static std::string& TrimLeft(std::string& str, const std::string& chars = "\t\n\v\f\r ");
    static std::string& TrimRight(std::string& str, const std::string& chars = "\t\n\v\f\r ");
    static std::string& Trim(std::string& str, const std::string& chars = "\t\n\v\f\r ");

    static std::vector<std::string> Split(const std::string &s,
                                          const char delim);
    static std::vector<std::string> Split(const std::string &text,
                                          const std::string &delims,
                                          bool ignore_empty = false);

    bool IgnoreCaseEquals(const std::string& str1, const std::string& str2);


    template <typename T, typename Token>
      static std::string Join(const std::vector<T> &v, const Token& token) {
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
      static std::vector<T> ParseArray(const std::string &s, const char delim) {
        std::vector<T> elems;
        std::istringstream f(s);
        std::string out;
        while (std::getline(f, out, delim)) {
          elems.push_back(Parse<T>(out));
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
      static bool ParseTo(const std::string& s, T& out) {
        std::istringstream iss(s);
        iss >> out; // = std::istringstream(str);
        return iss.good();
      }

    template <typename T>
      static T Parse(const std::string &s) {
        T result;
        std::istringstream(s) >> result;
        return result;
      }

    template<typename ...Args>
      static std::string Concat(Args&&... args) {
        std::ostringstream oss;
        __detail__::ConcatImpl(oss, std::forward<Args>(args)...);
        return oss.str();
      }
};

template <>
inline std::string StrUtil::Parse(const std::string &s) {
  return s;
}

template <>
inline const char* StrUtil::Parse(const std::string &s) {
  return s.c_str();
}

template <>
inline bool StrUtil::Parse(const std::string &s) {
  std::string new_s = s;
  ToLower(new_s);
  return new_s == "on" || new_s == "yes" || new_s == "true";
}

}  // namespace base
#endif
