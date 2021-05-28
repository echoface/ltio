#include "posting_list.h"

#include <algorithm>
#include <sstream>

namespace component {

void KSizePostingEntries::AddEntry(const Attr& attr, EntryId id) {
  posting_entries_[attr].push_back(id);
}

void KSizePostingEntries::MakeEntriesSorted() {
  for (auto& attr_entries : posting_entries_) {
    Entries& entries = attr_entries.second;
    std::sort(entries.begin(), entries.end());
  }
}

const Entries* KSizePostingEntries::GetEntryList(const Attr& attr) const {
  auto iter = posting_entries_.find(attr);
  if (iter == posting_entries_.end()) {
    return nullptr;
  }
  return &(iter->second);
}


void KSizePostingEntries::DumpPostingEntries(std::ostringstream& oss) const {
  for (auto& attr_entries : posting_entries_) {

    const Attr& attr = attr_entries.first;
    const Entries& entries = attr_entries.second;

    oss << attr.first << "#" << attr.second << ":[";
    bool is_first = true;
    for (auto& entry_id : entries) {
      uint64_t conj_id = EntryUtil::GetConjunctionId(entry_id);
      if (!is_first) {
        oss << ",";
      }
      oss << EntryUtil::ToString(entry_id);
      is_first =  false;
    }
    oss << "]\n";
  }
};


}
