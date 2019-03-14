#include <thread>
#include <unistd.h>
#include <iostream>
#include <glog/logging.h>
#include "metrics_stash.h"
#include <nlohmann/json.hpp>

namespace component {

StashQueueGroup::StashQueueGroup(uint32_t count)
  : queues_(count) {
  index_.store(0);
}

MetricsStash::MetricsStash()
  : running_(false),
    th_(NULL),
    last_report_time_(::time(NULL)),
    queue_groups_(std::thread::hardware_concurrency()) {
  std::atomic_store(&container_, RefMetricContainer(new MetricContainer()));
}

void MetricsStash::SetDelegate(Delegate* d) {
  delegate_ = d;
}

bool MetricsStash::Stash(MetricsItemPtr&& item) {
  StashQueue& queue = queue_groups_.NextQueue();
  return queue.enqueue(std::move(item));
}

void MetricsStash::StopSync() {
  running_ = false;
  if (th_ && th_->joinable()) {
    th_->join();
  }
  if (th_) {
    delete th_;
  }
  th_ = nullptr;
}

void MetricsStash::Start() {
  CHECK(!running_ && !th_);
  running_ = true;
  th_ = new std::thread(&MetricsStash::StashMain, this);
}

uint64_t MetricsStash::ProcessMetric() {
  uint64_t item_count = 0;

  RefMetricContainer container = std::atomic_load(&container_);

  for (uint32_t i = 0; i < queue_groups_.GroupSize(); i++) {

    StashQueue& queue = queue_groups_.At(i);

    std::vector<MetricsItemPtr> items(1000);
	  size_t count = queue.try_dequeue_bulk(items.begin(), items.size());
    if (count == 0) {
      continue;
    }

    for (size_t i = 0; i < count; i++) {
      item_count++;

      switch(items[i]->type_) {
        case MetricsType::kGauge:
          break;
        case MetricsType::kHistogram:
          container->MergeHistogram(items[i].get());
          break;
        default:
          break;
      }
    }
  }
  return item_count;
}

void MetricsStash::GenerateReport() {
  time_t now = ::time(NULL);
  if (now - last_report_time_ < 5) {
    return;
  }
  last_report_time_ = now;

  RefMetricContainer old = std::atomic_load(&container_);
  RefMetricContainer new_container(new MetricContainer());
  std::atomic_store(&container_, new_container);

  Json report;
  old->GenerateJsonReport(report);
  std::cout << report << std::endl;
}

void MetricsStash::StashMain() {

  while(running_) {

    uint32_t handle_count = ProcessMetric();

    if (handle_count == 0) {
      ::usleep(100000);
    }

    GenerateReport();
  }
}

}
