#ifndef BASE_CORO_BSTCTX_IMPL_H_H
#define BASE_CORO_BSTCTX_IMPL_H_H

#include <string.h>
#include <memory>
#include "base/logging.h"
#include "base/lt_macro.h"
#include "base/queue/linked_list.h"
#include "fcontext/fcontext.h"
#include "glog/logging.h"

namespace co {

class Coroutine;
using RefCoroutine = std::shared_ptr<Coroutine>;
using WeakCoroutine = std::weak_ptr<Coroutine>;

struct coro_context {
 fcontext_stack_t stack_;
 fcontext_transfer_t co_ctx_;
};

class Coroutine : public coro_context,
                  public base::LinkedList<Coroutine>::Node,
                  public EnableShared(Coroutine) {
public:
  using EntryFunc = pfn_fcontext;
  typedef fcontext_transfer_t(*OntopFunc)(fcontext_transfer_t);

  static RefCoroutine New() {
    return RefCoroutine(new Coroutine());
  }

  static RefCoroutine New(EntryFunc fn, void* data) {
    return RefCoroutine(new Coroutine(fn, data));
  }

  ~Coroutine() {
    VLOG(VTRACE) << "corotuine gone, " << CoInfo();
    CHECK(!Attatched());
    destroy_fcontext_stack(&stack_);
  }

private:
  Coroutine() {
    memset(&stack_, 0, sizeof(stack_));
    Reset(nullptr, nullptr);
  }

  Coroutine(EntryFunc fn, void* data) {
    CHECK(fn);

    memset(&stack_, 0, sizeof(stack_));
    stack_ = create_fcontext_stack(64 * 1024);

    Reset(fn, data);
    VLOG(VTRACE) << "corotuine born, " << CoInfo();
  }
public:

  void Reset(EntryFunc fn, void* data) {
    co_ctx_.data = data;
    if (stack_.sptr && stack_.ssize && fn) {
      co_ctx_.ctx = make_fcontext(stack_.sptr, stack_.ssize, fn);
    }
  }

  void Resume() {
    resume_id_++;
    co_ctx_ = jump_fcontext(co_ctx_.ctx, co_ctx_.data);
  }

  void ResumeWith(OntopFunc fn) {
    resume_id_++;
    co_ctx_ = ontop_fcontext(co_ctx_.ctx, co_ctx_.data, fn);
  }

  uint64_t MarkReady() { return ++resume_id_;}

  uint64_t ResumeID() const { return resume_id_; }

  Coroutine* SelfHolder() {
    self_ = shared_from_this();
    return this;
  }

  bool IsCoroZero() const{return stack_.sptr == nullptr;}

  void ReleaseSelfHolder() { self_.reset(); };

  WeakCoroutine AsWeakPtr() { return shared_from_this(); }

  std::string CoInfo() const {
    std::ostringstream oss;
    oss << "[co@" << this << "#" << resume_id_;
    if (stack_.sptr == nullptr) {
      oss << "#main]";
    } else {
      oss << "#coro]";
    }
    return oss.str();
  }
private:
  uint64_t resume_id_ = 0;

  std::shared_ptr<Coroutine> self_;

  DISALLOW_COPY_AND_ASSIGN(Coroutine);
};

}  // namespace base

#endif
