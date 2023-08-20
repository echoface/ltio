#ifndef _COMPONENT_LOG_METRICS_H_
#define _COMPONENT_LOG_METRICS_H_

#include <blockingconcurrentqueue.h>
#include <atomic>
#include <thread>
#include <vector>
#include "metrics_container.h"
#include "metrics_define.h"
#include "metrics_item.h"

namespace component {

typedef std::shared_ptr<MetricContainer> RefMetricContainer;

typedef moodycamel::BlockingConcurrentQueue<RefMetricContainer> HandleQueue;

class StashQueueGroup {
public:
  StashQueueGroup(uint32_t count);
  uint32_t GroupSize() const { return queues_.size(); }
  StashQueue& At(uint32_t idx) { return queues_.at(idx); };
  StashQueue& NextQueue() { return queues_[++index_ % queues_.size()]; }

private:
  std::atomic<uint32_t> index_;
  std::vector<StashQueue> queues_;
};

class MetricsStash : public MetricContainer::Provider {
public:
  typedef struct Handler {
    /* handle this metric, can do other things like prometheus monitor, or just
     * by judge the thread presure by check the metric birth time return true
     * mean metric had been handled, MetricStash will not handle again the stash
     * will continue sumup this metric when return false;
     * */
    virtual bool HandleMetric(const MetricsItem* item) { return false; };
  } Handler;

  void RegistDistArgs(const std::string&, const MetricDistArgs);

  MetricsStash();
  ~MetricsStash();
  void SetHandler(Handler* d);
  void EnableTurboMode(bool turbo);
  void SetReportInterval(uint32_t interval);

  void Start();
  void StopSync();

  bool Stash(MetricsItemPtr&& item);
  const MetricDistArgs* GetDistArgs(const std::string& name) override;

private:
  void StashMain();
  void TurboMain();
  void GenerateReport();
  uint64_t ProcessMetric(RefMetricContainer& c);

  bool running_;
  std::thread* stash_th_;
  uint32_t report_interval_;   // in second
  uint32_t last_report_time_;  // timestamp in second
  Handler* handler_ = nullptr;
  StashQueueGroup queue_groups_;
  RefMetricContainer container_;

  bool enable_turbo_;
  std::thread* turbo_th_;
  HandleQueue dispatch_quque_;
  HandleQueue merger_quque_;

  std::unordered_map<std::string, MetricDistArgs> dist_args_map_;
};

}  // namespace component
#endif
