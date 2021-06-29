#ifndef _LT_COMPONENT_BOOLEAN_INDEXER_H_
#define _LT_COMPONENT_BOOLEAN_INDEXER_H_

#include <algorithm>
#include <cassert>
#include <set>
#include <sstream>
#include <unordered_map>

#include "document.h"
#include "id_generator.h"
#include "posting_list.h"
#include "scanner_cursor.h"

namespace component {

class IndexScanner;
class BooleanIndexer {
public:
  static const Attr& WildcardAttr();

  BooleanIndexer();

  ~BooleanIndexer();

  void CompleteIndex();

  void DumpIndex(std::ostringstream& oss) const;

  void DumpIDMapping(std::ostringstream& oss) const;

  KSizePostingEntries* MutableIndexes(size_t k);
  const KSizePostingEntries* GetPostEntries(size_t k) const;

  FieldQueryValues ParseAssigns(const QueryAssigns& queries) const;

  std::vector<FieldCursorPtr> BuildFieldIterators(
      const size_t k_size,
      const FieldQueryValues& assigns) const;

  uint64_t GenUniqueID(const std::string& value);

  const size_t MaxKSize() const { return ksize_entries_.size(); }

private:
  friend IndexScanner;

  const Entries* wildcard_list_;

  std::vector<KSizePostingEntries> ksize_entries_;

  std::unordered_map<std::string, uint64_t> id_gen_;
};

typedef std::shared_ptr<BooleanIndexer> RefBooleanIndexer;

}  // namespace component
#endif
