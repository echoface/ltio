#ifndef _LT_COMPONENT_BOOLEAN_INDEXER_IDGEN_H_
#define _LT_COMPONENT_BOOLEAN_INDEXER_IDGEN_H_

#include <cstdint>
#include <string>
#include <vector>

namespace component {

typedef uint64_t EntryId;
typedef std::vector<EntryId> EntryIdList;
typedef std::pair<std::string, int> Attr;

uint64_t GenConjunctionId(uint32_t doc, uint8_t index, uint8_t size);
class ConjUtil {
public:
  //doc: docment id
  //idx: index of this conjunction in owned document
  //size: conjunction size (the count of include type booleanexpression)
  // |--empty(8)--|--empty(8)--|--doc(32)--|--index(8)--|--size(8)--|
  static uint64_t GenConjID(uint32_t doc, uint8_t index, uint8_t size);

  static uint32_t GetDocumentID(uint64_t conj_id);
  static uint32_t GetIndexInDoc(uint64_t conj_id);
  static uint32_t GetConjunctionSize(uint64_t conj_id);
};

class EntryUtil {
public:
  // conj_id: conjunction_id
  // |--doc(32)--|--index(8)--|--size(8)--|--empty(8)--|--empty(7)--|--exclude(1)--|
  static EntryId GenEntryID(uint64_t conj_id, bool exclude);
  static uint64_t HasExclude(const EntryId id);
  static uint64_t GetConjunctionId(const EntryId id);
};

}
#endif
