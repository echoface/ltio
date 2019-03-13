#ifndef _COMPONENT_LOG_METRICS_CONTAINER_H
#define _COMPONENT_LOG_METRICS_CONTAINER_H

#include <vector>
#include <string>
#include <unordered_map>
#include "metrics_item.h"
#include <nlohmann/json_fwd.hpp>

using Json = nlohmann::json;

namespace component {

struct MetricDist {
  MetricDist();
  MetricDist(uint32_t start);
  uint32_t Qps() const;
  uint32_t Duration() const;
  uint32_t CalculatePecentile(uint32_t percent) const;

  uint64_t counter;
  double min_value;
  double max_value;
  double total_value;
  uint32_t start_ts;
  std::vector<uint32_t> percentile_bucket;
};

void to_json(Json& j, const MetricDist& dist);

class MetricContainer {
public:
  MetricContainer();
  void MergeHistogram(const MetricsItem* item);

  bool GenerateJsonReport(Json& report);
private:
  const uint32_t start_time;
  std::unordered_map<std::string, MetricDist> datas_;
};

}
#endif
