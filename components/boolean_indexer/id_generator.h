#ifndef _LT_COMPONENT_BOOLEAN_INDEXER_IDGEN_H_
#define _LT_COMPONENT_BOOLEAN_INDEXER_IDGEN_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace component {

#define NULLENTRY 0xFFFFFFFFFFFFFFFF

typedef uint64_t EntryId;
typedef std::vector<EntryId> Entries;

typedef int64_t ValueID;
typedef std::vector<ValueID> ValueList;
typedef std::pair<std::string, ValueID> Attr;
typedef std::map<std::string, ValueList> FieldQueryValues;

class ConjUtil {
public:
  // doc: docment id
  // idx: index of this conjunction in owned document
  // size: conjunction size (the count of include type booleanexpression)
  // |--empty(8)--|--empty(8)--|--size(8)--|--index(8)--|--doc(32)--|
  static uint64_t GenConjID(uint32_t doc, uint8_t index, uint8_t size);
  static uint32_t GetDocumentID(uint64_t conj_id);
  static uint32_t GetIndexInDoc(uint64_t conj_id);
  static uint32_t GetConjunctionSize(uint64_t conj_id);
};

class EntryUtil {
public:
  // conj_id: conjunction_id
  //|-- conjunction_id --|--empty(8)--|--empty(7)--|--exclude(1)--|
  static EntryId GenEntryID(uint64_t conj_id, bool exclude);

  static bool IsInclude(const EntryId id);
  static bool IsExclude(const EntryId id);
  static uint32_t GetDocID(const EntryId id);
  static size_t GetConjSize(const EntryId id);
  static uint64_t GetConjunctionId(const EntryId id);
  static std::string ToString(const EntryId id);
  static std::string ToString(const Attr& attr);

};

}
#endif
