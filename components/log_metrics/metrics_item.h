#ifndef _COMPONENET_METRICS_ITEM_H_
#define _COMPONENET_METRICS_ITEM_H_

#include <memory>
#include <string>
#include <vector>

namespace component {

typedef enum MetricsOption {
  kOptNone = 0x0,
} MetricsOption;

enum class MetricsType {
  kGauge,      // key: max, min, avg, current
  kHistogram,  // key: qps(from counter), percentile, max, min,
               // avg(distrubution)
};

typedef std::pair<std::string, std::string> PropertyPair;

class MetricsItem {
public:
  MetricsItem(MetricsType type,
              const std::string& name,
              const int64_t val,
              MetricsOption opts);
  MetricsType type_;
  const int64_t value_;
  const std::string name_;
  const MetricsOption options_;
  std::vector<PropertyPair> proerties_;
};

typedef std::unique_ptr<MetricsItem> MetricsItemPtr;

}  // namespace component
#endif
