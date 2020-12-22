#ifndef ACO_COROUTINE_H_H_
#define ACO_COROUTINE_H_H_

#include <atomic>
#include <cinttypes>
#include <functional>
#include <memory>

#include "config.h"
#include <base/base_micro.h>
#include <base/closure/closure_task.h>
#include <thirdparty/libaco/aco.h>

namespace base {

class Coroutine;
typedef std::weak_ptr<Coroutine> WeakCoroutine;
typedef std::shared_ptr<Coroutine> RefCoroutine;

int64_t SystemCoroutineCount();
typedef void (*CoroEntry)();

class Coroutine :public EnableShared(Coroutine) {
public:
  friend class CoroRunner;

  static RefCoroutine CreateMain();
  static RefCoroutine Create(CoroEntry entry, Coroutine* main_co);

  ~Coroutine();
  CoroState Status() const {return state_;}
  uint64_t ResumeId() const {return resume_cnt_;}

  inline bool IsPaused() const {return state_ == CoroState::kPaused;}
  inline bool IsRunning() const {return state_ == CoroState::kRunning;}

  /*NOTE: this will switch to main coro, be care for call this
   * must ensure call it when coro_fn finish, can't use return*/
  void Exit();

  /*swich thread run context between main_coro and sub_coro*/
  void TransferTo(Coroutine* next);

  bool CanResume(int64_t resume_id);

  std::string StateToString() const;
private:
  Coroutine();
  Coroutine(CoroEntry entry, Coroutine* main_co);

  void SelfHolder(RefCoroutine& self);
  void SetCoroState(CoroState st) {state_ = st;}
  void ReleaseSelfHolder() {self_holder_.reset();};
  WeakCoroutine AsWeakPtr() {return shared_from_this();}
  bool is_main() const {return sstk_ == nullptr;}
private:
  CoroState state_;

  /* 此resume_cnt_ 只有在每次从暂停状态转换成running状态之后才会自增; 用于防止非正确的唤醒; eg:
   * coro running -> coro paused 之后有多个事件都可能唤醒他，其中一个已经唤醒过此coro，唤醒之后
   * 再次puased， 若此前另外一个事件唤醒coro， 将导致不正确的状态.*/
  uint64_t resume_cnt_;
  aco_t* coro_ = nullptr;
  aco_share_stack_t* sstk_ = nullptr;

  RefCoroutine self_holder_;
  DISALLOW_COPY_AND_ASSIGN(Coroutine);
};

}//end base
#endif
