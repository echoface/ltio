#ifndef _LT_COMPONENT_BOOLEAN_INDEX_PL_H_
#define _LT_COMPONENT_BOOLEAN_INDEX_PL_H_

#include <unordered_map>

#include "id_generator.h"

namespace component {

struct pair_hash {
  template <class T1, class T2>
  std::size_t operator()(const std::pair<T1, T2>& pair) const {
    return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
  }
};

typedef std::unordered_map<Attr, Entries, pair_hash> PostingList;

class KSizePostingEntries {
public:
  KSizePostingEntries() {}

  void MakeEntriesSorted();

  void AddEntry(const Attr& attr, EntryId id);

  const Entries* GetEntryList(const Attr& attr) const;

  void DumpPostingEntries(std::ostringstream& oss) const;

private:
  // Attr=>EntryIdList
  // eg: (age, 15) => [1, 2, 5, 9]
  PostingList posting_entries_;
};

}  // namespace component
#endif
