#ifndef _COMPONENET_METRICS_DEFINE_H_
#define _COMPONENET_METRICS_DEFINE_H_

#include <thirdparty/cameron_queue/concurrentqueue.h>

namespace component {

class MetricsItem;

class LogMetricsQueueTraits : public moodycamel::ConcurrentQueueDefaultTraits {
public:
  static const size_t BLOCK_SIZE = 1024;
  static constexpr size_t MAX_SUBQUEUE_SIZE = 16 * 1024 * 1024;
};

typedef moodycamel::ConcurrentQueue<MetricsItemPtr, LogMetricsQueueTraits>
    StashQueue;

}  // namespace component

#endif
