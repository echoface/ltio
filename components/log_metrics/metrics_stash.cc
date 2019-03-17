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

MetricsStash::~MetricsStash() {
  if (running_) StopSync();
}

MetricsStash::MetricsStash()
  : running_(false),
    stash_th_(NULL),
    report_interval_(30),
    last_report_time_(::time(NULL)),
    queue_groups_(std::thread::hardware_concurrency()),
    enable_turbo_(false),
    turbo_th_(NULL) {

  RefMetricContainer container(new MetricContainer(this));
  std::atomic_store(&container_, container);
}

void MetricsStash::SetHandler(Handler* d) {
  handler_ = d;
}

void MetricsStash::EnableTurboMode(bool turbo) {
  enable_turbo_ = turbo;
}

void MetricsStash::SetReportInterval(uint32_t interval) {
  report_interval_ = interval >= 5 ? interval : 5;
}

bool MetricsStash::Stash(MetricsItemPtr&& item) {
  StashQueue& queue = queue_groups_.NextQueue();
  return queue.enqueue(std::move(item));
}

void MetricsStash::StopSync() {
  running_ = false;

  if (turbo_th_) {
    if (turbo_th_->joinable()) turbo_th_->join();
    delete turbo_th_;
    turbo_th_ = nullptr;
  }

  if (stash_th_) {
    if (stash_th_->joinable()) stash_th_->join();
    delete stash_th_;
    stash_th_ = nullptr;
  }
}

void MetricsStash::Start() {
  CHECK(!running_ && !stash_th_);
  running_ = true;

  if (enable_turbo_) {//? dynamic turbo mode?
    turbo_th_ = new std::thread(&MetricsStash::TurboMain, this);
  }
  stash_th_ = new std::thread(&MetricsStash::StashMain, this);
}

void MetricsStash::TurboMain() {
  while(running_) {

    RefMetricContainer container;
    dispatch_quque_.wait_dequeue_timed(container, 1000000);
    if (!container) continue;
    uint32_t start = ::time(NULL);
    uint64_t handle_count = 0;
    do {
      handle_count = ProcessMetric(container);
    } while(handle_count != 0 && ::time(NULL) - start < 1);
    merger_quque_.enqueue(std::move(container));
  }
}

uint64_t MetricsStash::ProcessMetric(RefMetricContainer& container) {
  uint64_t item_count = 0;

  for (uint32_t i = 0; i < queue_groups_.GroupSize(); i++) {

    StashQueue& queue = queue_groups_.At(i);

    std::vector<MetricsItemPtr> items(10000);
	  size_t count = queue.try_dequeue_bulk(items.begin(), items.size());
    if (count == 0) {
      continue;
    }

    for (size_t i = 0; i < count; i++) {
      item_count++;
      MetricsItem* item = items[i].get();

      if (handler_ && handler_->HandleMetric(item)) {
       continue;
      }

      switch(items[i]->type_) {
        case MetricsType::kGauge:
          container->HandleGuage(item);
          break;
        case MetricsType::kHistogram:
          container->HandleHistogram(item);
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
  RefMetricContainer new_container(new MetricContainer(this));
  std::atomic_store(&container_, new_container);

  std::ostringstream oss;
  old->GenerateJsonLineReport(oss);
  std::cout << oss.str() << std::endl;
}

void MetricsStash::StashMain() {

  while(running_) {
    if (enable_turbo_) { //active turbo
      RefMetricContainer turbo_container(new MetricContainer(this));
      dispatch_quque_.enqueue(std::move(turbo_container));
    }

    RefMetricContainer main_container = std::atomic_load(&container_);
    uint32_t handle_count = ProcessMetric(main_container);

    if (handle_count == 0) {
      ::usleep(100000);
    }

    if (enable_turbo_) { //merge turbo result
      RefMetricContainer turbo_container;
      if (merger_quque_.try_dequeue(turbo_container) && turbo_container) {
        main_container->Merge(*turbo_container);
      }
    }
    GenerateReport();
  }
}

void MetricsStash::RegistDistArgs(const std::string& name, const MetricDistArgs arg) {
  if (running_) {
    LOG(FATAL) << "FATAL: must regist before stash running";
    return;
  }
  dist_args_map_[name] = arg;
}

const MetricDistArgs* MetricsStash::GetDistArgs(const std::string& name) {
  auto iter = dist_args_map_.find(name);
  if (iter == dist_args_map_.end()) {
    return nullptr;
  }
  return &(iter->second);
}

}
