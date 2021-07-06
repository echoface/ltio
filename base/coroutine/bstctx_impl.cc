#include <string.h>
#include "base/closure/closure_task.h"
#include "fcontext/fcontext.h"

namespace base {

class Coroutine {
public:
  static void __main__(fcontext_transfer_t from) {
    Coroutine* coro = (Coroutine*)from.data;
    while(1) {
      TaskBasePtr task;
      task->Run();
    }
    from = coro->from_;
    delete coro;
    jump_fcontext(from.ctx, from.data);
  }

  Coroutine()
    : stack_(create_fcontext_stack(0)) {
    Reset();
  }

  ~Coroutine() {
    destroy_fcontext_stack(&stack_);
  }

  void Reset() {
    status_ = 0;
    ctx_.data = nullptr;
    memset(&from_, 0, sizeof(fcontext_transfer_t));
    ctx_.ctx = make_fcontext(stack_.sptr, stack_.ssize, Coroutine::__main__);
  }

  void Yield() {
    from_ = jump_fcontext(from_.ctx, from_.data);
  }

  void Resume() {
    status_ = 3;
    // 从main thread 跳到coro中
    ctx_ = jump_fcontext(ctx_.ctx, ctx_.data);
  }

private:
  int status_ = 0; //0: paused 1:running 3:done
  fcontext_stack_t stack_;
  fcontext_transfer_t ctx_;
  fcontext_transfer_t from_;
};

}  // namespace base
