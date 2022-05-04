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
class BeIndexerBuilder;

class BooleanIndexer {
public:
  BooleanIndexer();

  ~BooleanIndexer();

  void DumpIndex(std::ostringstream& oss) const;

  FieldCursorPtr GenWildcardCursor() const;

  FieldMeta* GetMeta(const std::string& field) const;

  EntriesContainer* GetContainer(const std::string field) const;

private:
  friend IndexScanner;
  friend BeIndexerBuilder;

  bool SetMeta(const std::string& field, FieldMetaPtr meta);

  bool CompleteIndex();

  void AddWildcardEntry(const EntryId eid);

private:

  Entries wildcard_list_;

  std::map<std::string, FieldMetaPtr> field_meta_;

  // here register a speical key: ___default___ as the
  // default container for fields without specifing container
  std::map<std::string, EntriesContainerPtr> containers_;
};

using RefBooleanIndexer = std::shared_ptr<BooleanIndexer>;
using BooleanIndexerPtr = std::unique_ptr<BooleanIndexer>;

}  // namespace component
#endif
