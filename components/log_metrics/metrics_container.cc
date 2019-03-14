#include <glog/logging.h>
#include <time.h>
#include "metrics_container.h"
#include <nlohmann/json.hpp>

const static uint32_t kDistRange = 1000000;
const static uint32_t kPercentileBuckets = 10000;
const static uint32_t kPercentilePrecise = kDistRange /  kPercentileBuckets;

namespace component {

MetricDist::MetricDist()
  : counter(0),
    min_value(0),
    max_value(0),
    total_value(0),
    start_ts(::time(NULL)),
    percentile_bucket(kPercentileBuckets) {
}
MetricDist::MetricDist(uint32_t start)
  : counter(0),
    min_value(0),
    max_value(0),
    total_value(0),
    start_ts(start),
    percentile_bucket(kPercentileBuckets) {
}

uint32_t MetricDist::Duration() const {
  return ::time(NULL) - start_ts;
}

uint32_t MetricDist::Qps() const {
  uint32_t duration = Duration();
  return duration > 0 ? counter / duration : counter;
}

uint32_t MetricDist::CalculatePecentile(uint32_t percent) const {
  uint32_t count = counter * percent / 100;
  uint64_t sum = 0;
  uint32_t i = 0;
  for (;sum < count && i < percentile_bucket.size(); i++) {
    sum += percentile_bucket[i];
  }
  return kPercentilePrecise * i;
}

std::vector<uint32_t> MetricDist::CalculatePecentile(std::vector<uint32_t> percent) const {
  std::vector<uint32_t> result(percent.size());
  std::sort(percent.begin(), percent.end());

  uint64_t sum = 0;
  uint32_t bucket = 0;
  uint32_t result_i = 0;
  for (; bucket < percentile_bucket.size() && result_i < percent.size(); bucket++) {
    sum += percentile_bucket[bucket];
    if (sum > (counter / 100) * percent[result_i]) {
      result[result_i] = kPercentilePrecise * bucket;
      result_i++;
    }
  }
  return std::move(result);
}

void to_json(Json& j, const MetricDist& dist) {

  auto percentiles = dist.CalculatePecentile({50, 80, 90, 95, 99});
  std::ostringstream oss;
  oss << 50 << ":" << percentiles[0] << ", "
      << 80 << ":" << percentiles[1] << ", "
      << 90 << ":" << percentiles[2] << ", "
      << 95 << ":" << percentiles[3] << ", "
      << 99 << ":" << percentiles[4];

  j["dist"] = oss.str();
  /*
  //Json percentile;
  percentile["99"] = percentiles[4];
  percentile["95"] = percentiles[3];
  percentile["90"] = percentiles[2];
  percentile["80"] = percentiles[1];
  percentile["50"] = percentiles[0];
  j["dist"] = percentile;
  */

  j["qps"] = dist.Qps();
  j["sum"] = dist.total_value;
  j["min"] = dist.min_value;
  j["max"] = dist.max_value;
  if (dist.counter > 0) {
    j["avg"] = uint32_t(dist.total_value / dist.counter);
  }
}

MetricContainer::MetricContainer()
  : start_time(::time(NULL)) {
}

void MetricContainer::MergeHistogram(const MetricsItem* item) {
  CHECK(item->type_ == MetricsType::kHistogram);

  auto iter = datas_.find(item->name_);
  if (iter == datas_.end()) {
    auto pair = datas_.insert(std::make_pair(item->name_, MetricDist(start_time)));
    iter = pair.first;
  }
  auto& dist = datas_[item->name_];
  dist.counter++;
  dist.min_value = std::min(dist.min_value, double(item->value_));
  dist.max_value = std::max(dist.max_value, double(item->value_));
  dist.total_value += item->value_;

  if (item->value_ >= kDistRange) {
    dist.percentile_bucket[kPercentileBuckets - 1]++;
  } else {
    uint32_t pos = item->value_ / kPercentilePrecise;
    dist.percentile_bucket[pos]++;
  }
}

bool MetricContainer::GenerateJsonReport(Json& report) {
  for (auto& pair : datas_) {
    report[pair.first] = pair.second;
  }
  return true;
}

};
