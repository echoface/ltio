#include "common_container.h"

#include <algorithm>
#include <sstream>

#include "fmt/format.h"

namespace component {

bool CommonContainer::IndexingToken(const FieldMeta* meta,
                                    const EntryId eid,
                                    const Token* token) {
  if (token->BadToken()) {
    return false;
  }

  for (int64_t value : token->Int64()) {
    Attr attr = {meta->name, value};
    values_[attr].push_back(eid);
  }
  return true;
}

EntriesList CommonContainer::RetrieveEntries(const FieldMeta* meta,
                                             const Token* token) const {
  EntriesList results;
  for (int64_t value : token->Int64()) {
    Attr attr(meta->name, value);
    auto iter = values_.find(attr);
    if (iter == values_.end()) {
      continue;
    }
    results.push_back(&(iter->second));
  }
  return results;
}

bool CommonContainer::CompileEntries() {
  for (auto& pair : values_) {
    sum_len_ += pair.second.size();
    if (max_len_ < pair.second.size()) {
      max_len_ = pair.second.size();
    }
    std::sort(pair.second.begin(), pair.second.end());
  }
  return true;
};

void CommonContainer::DumpEntries(std::ostringstream& oss) const {
  oss << "+++++ start dump common container entries ++++++++++\n";
  for (auto& kvs : values_) {
    oss << "<" << kvs.first.first << "," << kvs.first.second << ">:";
    // fmt::format("{}", fmt::join(kvs.second, ","));
    for (auto iter = kvs.second.begin(); iter != kvs.second.end(); iter++) {
      if (iter != kvs.second.begin()) {
        oss << ",";
      }
      oss << EntryUtil::ToString(*iter);
    }
    oss << "\n";
  }
  oss << "++++++++++++++++ end all entries +++++++++++++++++++\n";
}

}  // end namespace component
