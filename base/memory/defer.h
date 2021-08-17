#ifndef _LT_BASE_DEFER_FUNC_H_
#define _LT_BASE_DEFER_FUNC_H_

#include <functional>
#include <utility>
namespace base {

namespace __detail__ {
template <typename F, typename... T>
struct Defer {
  Defer(F&& f, T&&... t)
    : f(std::bind(std::forward<F>(f), std::forward<T>(t)...)) {}
  Defer(Defer&& o) noexcept : f(std::move(o.f)) {}
  ~Defer() { f(); }

  using ResultType = typename std::result_of<typename std::decay<F>::type(
      typename std::decay<T>::type...)>::type;
  std::function<ResultType()> f;
};

template <typename F, typename... T>
struct CancelableDefer {
  CancelableDefer(F&& f, T&&... t)
    : cancel(false),
      f(std::bind(std::forward<F>(f), std::forward<T>(t)...)) {}

  CancelableDefer(CancelableDefer&& o) noexcept : f(std::move(o.f)) {}

  ~CancelableDefer() {
    if (!cancel) {
      f();
    }
  }

  void Cancel() { cancel = true; };

  using ResultType = typename std::result_of<typename std::decay<F>::type(
      typename std::decay<T>::type...)>::type;
  bool cancel = false;
  std::function<ResultType()> f;
};

}  // namespace __detail__

template <typename F, typename... T>
__detail__::Defer<F, T...> MakeDefer(F&& f, T&&... t) {
  return __detail__::Defer<F, T...>(std::forward<F>(f), std::forward<T>(t)...);
}

template <typename F, typename... T>
__detail__::CancelableDefer<F, T...> MakeCancelableDefer(F&& f, T&&... t) {
  return __detail__::CancelableDefer<F, T...>(std::forward<F>(f),
                                              std::forward<T>(t)...);
}

}  // namespace base
#endif
