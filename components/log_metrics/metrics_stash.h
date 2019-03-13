#ifndef _COMPONENT_LOG_METRICS_H_
#define _COMPONENT_LOG_METRICS_H_

#include <vector>
#include <atomic>
#include <thread>
#include "metrics_item.h"
#include "metrics_define.h"
#include "metrics_container.h"

namespace component {

typedef std::shared_ptr<MetricContainer> RefMetricContainer;

class StashQueueGroup {
public:
  StashQueueGroup(uint32_t count);
  uint32_t GroupSize() const {return queues_.size();}
  StashQueue& At(uint32_t idx) {return queues_.at(idx);};
  StashQueue& NextQueue() {return queues_[++index_ % queues_.size()];}
private:
  std::atomic<uint32_t> index_;
  std::vector<StashQueue> queues_;
};

class MetricsStash {
public:
  typedef struct Delegate {
  }Delegate;
  MetricsStash();
  void SetDelegate(Delegate* d);
  void Start();
  void StopSync();

  bool Stash(MetricsItemPtr&& item);
private:
  void StashMain();
  void GenReport();
  uint64_t ProcessMetric();

  bool running_;
  std::thread* th_;
  uint32_t last_report_time_;
  Delegate* delegate_ = nullptr;
  StashQueueGroup queue_groups_;
  RefMetricContainer container_;
};

}
#endif
