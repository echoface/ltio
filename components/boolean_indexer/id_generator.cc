#include "id_generator.h"

namespace component {

// |--empty(8)--|--empty(8)--|--size(8)--|--index(8)--|--doc(32)--|
uint64_t ConjUtil::GenConjID(uint32_t doc, uint8_t idx, uint8_t size) {
  return (uint64_t(size) << 40) | (uint64_t(idx) << 32) | uint64_t(doc);
}

uint32_t ConjUtil::GetDocumentID(uint64_t conj_id) {
  return conj_id & 0xFFFFFFFF;
}

uint32_t ConjUtil::GetIndexInDoc(uint64_t conj_id) {
  return (conj_id >> 32) & 0xFF;
}
uint32_t ConjUtil::GetConjunctionSize(uint64_t conj_id) {
  return (conj_id >> 40) & 0xFF;
}

//|-- conjunction_id --|--empty(8)--|--empty(7)--|--exclude(1)--|
EntryId EntryUtil::GenEntryID(uint64_t conj_id, bool exclude) {
  uint64_t entry_id = (conj_id << 16);
  return exclude ? entry_id : (entry_id | 0x01);
}

bool EntryUtil::HasExclude(const EntryId id) {
  return (id & 0x01) == 0x00;
}

uint64_t EntryUtil::GetConjunctionId(const EntryId id) {
  return id >> 16;
}

}
