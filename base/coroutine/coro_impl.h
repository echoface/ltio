#ifndef COROUTINE_H_H_
#define COROUTINE_H_H_

#include <cinttypes>
#include <atomic>
#include <memory>

#include "config.h"
#include <functional>
#include <base/base_micro.h>
#include <thirdparty/libcoro/coro.h>
#include <base/closure/closure_task.h>

namespace base {

class Coroutine;
typedef std::weak_ptr<Coroutine> WeakCoroutine;
typedef std::shared_ptr<Coroutine> RefCoroutine;

int64_t SystemCoroutineCount();
typedef void (*LibCoroEntry)(void* arg);

class Coroutine : public coro_context,
                  public EnableShared(Coroutine) {
public:
  friend class CoroRunner;

  static RefCoroutine CreateMain();
  static RefCoroutine Create(LibCoroEntry entry);
  ~Coroutine();

  CoroState Status() const {return state_;}
  uint64_t ResumeId() const {return resume_cnt_;}

  inline bool IsPaused() const {return state_ == CoroState::kPaused;}
  inline bool IsRunning() const {return state_ == CoroState::kRunning;}


  std::string StateToString() const;
  void TransferTo(Coroutine* next);
  bool CanResume(int64_t resume_id);
private:
  Coroutine(); //main coro
  Coroutine(LibCoroEntry entry); //sub coro

  void Reset();
  void SelfHolder(RefCoroutine& self);
  void SetCoroState(CoroState st) {state_ = st;}
  void ReleaseSelfHolder() {self_holder_.reset();};
  std::weak_ptr<Coroutine> AsWeakPtr() {return shared_from_this();}
  bool is_main() const {return stack_.ssze == 0;}
private:
  CoroState state_;
  coro_stack stack_;

  /* 此resume_cnt_ 只有在每次从暂停状态转换成running状态之后才会自增; 用于防止非正确的唤醒; eg:
   * coro running -> coro paused 之后有多个事件都可能唤醒他，其中一个已经唤醒过此coro，唤醒之后
   * 再次puased， 若此前另外一个事件唤醒coro， 将导致不正确的状态.*/
  uint64_t resume_cnt_;

  RefCoroutine self_holder_;
  DISALLOW_COPY_AND_ASSIGN(Coroutine);
};

}//end base
#endif
