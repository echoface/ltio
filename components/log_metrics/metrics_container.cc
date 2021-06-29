#include "metrics_container.h"
#include <glog/logging.h>
#include <time.h>
#include <nlohmann/json.hpp>

namespace component {

const static MetricDistArgs kDefaultDistArgs = {.dist_precise = 100,
                                                .dist_range_min = 0,
                                                .dist_range_max = 1000000};

void MetricBase::UpdateMinMax(int64_t v) {
  min_value = std::min(min_value, v);
  max_value = std::max(max_value, v);
}

// >> code for metric guage
MetricGuage::MetricGuage()
  : MetricBase(::time(NULL), MetricsType::kGauge), latest_value(0) {}
MetricGuage::MetricGuage(uint32_t ts)
  : MetricBase(ts, MetricsType::kGauge), latest_value(0) {}

bool MetricGuage::Merge(const MetricBase* other) {
  MetricGuage* guage = (MetricGuage*)other;
  if (!guage)
    return false;

  counter += other->counter;
  total_value += other->total_value;
  min_value = std::min(other->min_value, min_value);
  max_value = std::max(other->max_value, max_value);
  latest_value = guage->latest_value;
  return true;
}

void to_json(Json& j, const MetricGuage& guage) {
  j["max"] = guage.max_value;
  j["min"] = guage.min_value;
  j["count"] = guage.counter;
  j["avg"] = guage.counter > 0 ? guage.total_value / guage.counter : 0;
}

// >>>> code of metric distribution
MetricDist::MetricDist()
  : MetricBase(::time(NULL), MetricsType::kHistogram), args(kDefaultDistArgs) {
  double range = args.dist_range_max - args.dist_range_min;
  uint32_t buckets = std::ceil(range / args.dist_precise);
  percentile_bucket.resize(buckets);
}

MetricDist::MetricDist(uint32_t start, const MetricDistArgs& args)
  : MetricBase(start, MetricsType::kHistogram), args(args) {
  double range = args.dist_range_max - args.dist_range_min;
  uint32_t buckets = std::ceil(range / args.dist_precise);
  percentile_bucket.resize(buckets);
}

bool MetricDist::Merge(const MetricBase* item) {
  if (!item)
    return false;

  MetricDist* other = (MetricDist*)item;

  if (BucketSize() == other->BucketSize()) {
    return false;
  }
  counter += other->counter;
  total_value += other->total_value;
  min_value = std::min(other->min_value, min_value);
  max_value = std::max(other->max_value, max_value);
  // uint32_t start_ts will not merge
  for (size_t i = 0; i < BucketSize(); i++) {
    percentile_bucket[i] += other->percentile_bucket[i];
  }
  return true;
}

void MetricDist::IncrBucketByValue(const int64_t value) {
  if (value >= args.dist_range_max) {
    percentile_bucket.back()++;
  } else if (value <= args.dist_range_min) {
    percentile_bucket.front()++;
  } else {
    uint32_t pos = value / args.dist_precise;
    percentile_bucket[pos]++;
  }
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
  for (; sum < count && i < BucketSize(); i++) {
    sum += percentile_bucket[i];
  }
  return args.dist_precise * i;
}

std::vector<uint32_t> MetricDist::CalculatePecentile(
    std::vector<uint32_t> percent) const {
  std::vector<uint32_t> result(percent.size());
  std::sort(percent.begin(), percent.end());

  uint64_t sum = 0;
  uint32_t bucket = 0;
  uint32_t result_i = 0;
  for (; bucket < BucketSize() && result_i < percent.size(); bucket++) {
    sum += percentile_bucket[bucket];
    if (sum > (counter / 100) * percent[result_i]) {
      result[result_i] = args.dist_precise * bucket;
      result_i++;
    }
  }
  return result;
}

void to_json(Json& j, const MetricDist& dist) {
  auto percentiles = dist.CalculatePecentile({50, 80, 90, 95, 99});
  std::ostringstream oss;
  oss << 50 << ":" << percentiles[0] << ", " << 80 << ":" << percentiles[1]
      << ", " << 90 << ":" << percentiles[2] << ", " << 95 << ":"
      << percentiles[3] << ", " << 99 << ":" << percentiles[4];
  j["dist"] = oss.str();

  j["qps"] = dist.Qps();
  j["sum"] = dist.total_value;
  j["min"] = dist.min_value;
  j["max"] = dist.max_value;
  if (dist.counter > 0) {
    j["avg"] = uint32_t(dist.total_value / dist.counter);
  }
}
// >>>> code of matric distribution

MetricContainer::MetricContainer(Provider* provider)
  : start_time(::time(NULL)), provider_(provider) {}
MetricContainer::~MetricContainer() {
  for (auto kv : datas_) {
    delete kv.second;
    kv.second = NULL;
  }
}

void MetricContainer::Merge(const MetricContainer& other) {
  for (auto& pair : other.datas_) {
    switch (pair.second->Type()) {
      case MetricsType::kGauge:
        GetGuage(pair.first)->Merge(pair.second);
        break;
      case MetricsType::kHistogram:
        GetHistogram(pair.first)->Merge(pair.second);
        break;
      default:
        break;
    }
  }
}

void MetricContainer::HandleGuage(const MetricsItem* item) {
  CHECK(item->type_ == MetricsType::kGauge);
  MetricGuage* guage = GetGuage(item->name_);
  if (guage == nullptr) {
    return;
  }
  guage->counter++;
  guage->UpdateMinMax(item->value_);
  guage->total_value += item->value_;
  guage->latest_value = item->value_;
}

void MetricContainer::HandleHistogram(const MetricsItem* item) {
  CHECK(item->type_ == MetricsType::kHistogram);

  MetricDist* dist = NULL;
  auto iter = datas_.find(item->name_);
  if (iter == datas_.end()) {
    dist = GetHistogram(item->name_);
  } else {
    dist = (MetricDist*)iter->second;
  }
  if (dist == nullptr) {
    return;
  }

  dist->counter++;
  dist->UpdateMinMax(item->value_);
  dist->total_value += item->value_;
  dist->IncrBucketByValue(item->value_);
}

bool MetricContainer::GenerateJsonReport(Json& report) {
  for (auto& pair : datas_) {
    switch (pair.second->Type()) {
      case MetricsType::kGauge:
        report[pair.first] = *(MetricGuage*)(pair.second);
        break;
      case MetricsType::kHistogram:
        report[pair.first] = *(MetricDist*)(pair.second);
        break;
      default:
        break;
    }
  }
  return true;
}

bool MetricContainer::GenerateJsonLineReport(std::ostringstream& oss) {
  oss << "[Metrics] start: metrics report >>>>> @" << ::time(NULL) << "\n";
  for (auto& pair : datas_) {
    Json json_data;
    switch (pair.second->Type()) {
      case MetricsType::kGauge:
        json_data = *(MetricGuage*)(pair.second);
        oss << pair.first << ":" << json_data << "\n";
        // report[pair.first] = *(MetricGuage*)(pair.second);
        break;
      case MetricsType::kHistogram:
        // report[pair.first] = *(MetricDist*)(pair.second);
        json_data = *(MetricDist*)(pair.second);
        oss << pair.first << ":" << json_data << "\n";
        break;
      default:
        break;
    }
  }
  oss << "[Metrics] end: metrics report  <<<<<<< \n";
  return true;
}

bool MetricContainer::GenerateFileReport(const std::string full_paht) {
  return false;
}

MetricGuage* MetricContainer::GetGuage(const std::string& name) {
  auto iter = datas_.find(name);
  if (iter != datas_.end()) {
    return (MetricGuage*)iter->second;
  }
  MetricGuage* new_guage = new MetricGuage(start_time);
  datas_[name] = new_guage;
  return new_guage;
}

MetricDist* MetricContainer::GetHistogram(const std::string& name) {
  auto iter = datas_.find(name);
  if (iter != datas_.end()) {
    return (MetricDist*)iter->second;
  }

  const MetricDistArgs* arg = &kDefaultDistArgs;
  if (provider_) {
    auto provider_arg = provider_->GetDistArgs(name);
    if (provider_arg) {
      arg = provider_arg;
    }
  }
  MetricDist* dist = new MetricDist(start_time, *arg);
  datas_[name] = dist;
  return dist;
}

};  // namespace component
