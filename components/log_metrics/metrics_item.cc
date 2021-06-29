#include "metrics_item.h"

namespace component {

MetricsItem::MetricsItem(MetricsType type,
                         const std::string& name,
                         const int64_t val,
                         MetricsOption opts)
  : type_(type), value_(val), name_(name), options_(opts) {}

};  // namespace component
