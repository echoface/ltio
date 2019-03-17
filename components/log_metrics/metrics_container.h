#ifndef _COMPONENT_LOG_METRICS_CONTAINER_H
#define _COMPONENT_LOG_METRICS_CONTAINER_H

#include <vector>
#include <string>
#include <unordered_map>
#include "metrics_item.h"
#include <nlohmann/json_fwd.hpp>
#include <initializer_list>


using Json = nlohmann::json;

namespace component {

typedef struct MetricDistArgs {
  int64_t dist_precise;
  int64_t dist_range_min;
  int64_t dist_range_max;
} MetricDistArgs;

struct MetricBase {
  MetricBase(uint32_t ts, MetricsType t):type(t), start_ts(ts){}
  virtual ~MetricBase(){};
  void UpdateMinMax(int64_t v);
  MetricsType Type() const {return type;};
  virtual bool Merge(const MetricBase* other) = 0;

  MetricsType type;
  uint64_t counter = 0;
  int64_t min_value = 0;
  int64_t max_value = 0;
  int64_t total_value = 0;
  uint32_t start_ts;
};

struct MetricGuage : MetricBase {
  MetricGuage();
  MetricGuage(uint32_t ts);
  bool Merge(const MetricBase* other) override;
  int64_t latest_value = 0;
};
void to_json(Json& j, const MetricGuage& dist);

struct MetricDist : MetricBase {
  MetricDist();
  MetricDist(uint32_t start, const MetricDistArgs& arg);

  bool Merge(const MetricBase* other) override;
  void IncrBucket(size_t index, uint32_t count = 1);
  void IncrBucketByValue(const int64_t value);

  uint32_t Qps() const;
  uint32_t Duration() const;
  uint32_t CalculatePecentile(uint32_t percent) const;
  std::vector<uint32_t> CalculatePecentile(std::vector<uint32_t> percent) const;
  size_t BucketSize() const {return percentile_bucket.size();};

  std::vector<uint32_t> percentile_bucket;
  const MetricDistArgs args;
};
void to_json(Json& j, const MetricDist& dist);

class MetricContainer {
public:
  struct Provider {
    virtual const MetricDistArgs* GetDistArgs(const std::string&) = 0;
  };

  MetricContainer(Provider*);
  ~MetricContainer();
  void Merge(const MetricContainer& other);

  void HandleGuage(const MetricsItem* item);
  void HandleHistogram(const MetricsItem* item);

  bool GenerateJsonReport(Json& report);
  bool GenerateJsonLineReport(std::ostringstream& oss);
  bool GenerateFileReport(const std::string full_paht);
private:
  /* craete new dist item and insert to map, return null
   * when failed insert to map*/
  MetricGuage* GetGuage(const std::string& name);
  MetricDist* GetHistogram(const std::string& name);

  const uint32_t start_time;
  Provider* provider_;
  std::unordered_map<std::string, MetricBase*> datas_;
};

}
#endif
