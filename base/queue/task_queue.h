#ifndef BASE_CLOSURE_TASK_QUEUE_H_H
#define BASE_CLOSURE_TASK_QUEUE_H_H

#include "base/closure/closure_task.h"
#include <thirdparty/cameron_queue/concurrentqueue.h>

namespace base {

struct TaskQueueTraits : moodycamel::ConcurrentQueueDefaultTraits {
	static const size_t BLOCK_SIZE = 1024;
  static const size_t IMPLICIT_INITIAL_INDEX_SIZE = 32;
};
typedef moodycamel::ConcurrentQueue<TaskBasePtr, TaskQueueTraits> ConcurrentTaskQueue;

}
#endif


