#ifndef _LT_COMPONENT_BOOLEAN_INDEX_PL_H_
#define _LT_COMPONENT_BOOLEAN_INDEX_PL_H_

#include <unordered_map>

#include "document.h"
#include "id_generator.h"
#include "parser/parser.h"

namespace component {

struct pair_hash {
  template <class T1, class T2>
  std::size_t operator()(const std::pair<T1, T2>& pair) const {
    return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
  }
};

typedef std::unordered_map<Attr, Entries, pair_hash> PostingList;

class FieldMeta {
public:
  std::string name;

  RefExprParser parser;

};
// alias here bz perfer client/dever use it like this
using FieldMetaPtr = std::unique_ptr<FieldMeta>;
using RefFieldMeta = std::shared_ptr<FieldMeta>;

class EntriesContainer {
public:
  virtual ~EntriesContainer(){};

  virtual bool IndexingToken(const FieldMeta* meta,
                             const EntryId eid,
                             const Token* expr) = 0;

  virtual EntriesList RetrieveEntries(const FieldMeta* meta,
                                      const Token* attrs) const = 0;

  virtual bool CompileEntries() = 0;

  virtual void DumpEntries(std::ostringstream& oss) const = 0;
};
// why here: perfer client/dever use it like this
using EntriesContainerPtr = std::unique_ptr<EntriesContainer>;
using RefEntriesContainer = std::shared_ptr<EntriesContainer>;

}  // namespace component
#endif
