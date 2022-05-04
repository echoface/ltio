#include <cstdint>
#include <sstream>

#include "id_generator.h"
#include "posting_list.h"

namespace component {

// |--empty(4)--|--size(8)--|--index(8)--|--negSign(1)--|--doc(43)--|
uint64_t ConjUtil::GenConjID(int64_t doc, uint8_t idx, uint8_t size) {
  if (doc < 0) {
    doc = (uint64_t(1) << 43) | uint64_t(-doc);
  }
  return (uint64_t(size) << 52) | (uint64_t(idx) << 44) | uint64_t(doc);
}

int64_t ConjUtil::GetDocumentID(uint64_t conj_id) {
  int64_t doc = conj_id & 0x7FFFFFFFFFF;
  int neg = (conj_id >> 43) & 0x1;
  return neg ? -doc : doc;
}

uint8_t ConjUtil::GetIndexInDoc(uint64_t conj_id) {
  return (conj_id >> 44) & 0xFF;
}

uint8_t ConjUtil::GetConjunctionSize(uint64_t conj_id) {
  return (conj_id >> 52) & 0xFF;
}

//|-- conjunction_id --|--empty(3)--|--flag(1)--|
EntryId EntryUtil::GenEntryID(uint64_t conj_id, bool exclude) {
  uint64_t entry_id = (conj_id << 4);
  return exclude ? entry_id : (entry_id | 0x01);
}

bool EntryUtil::IsInclude(const EntryId id) {
  return (id & 0x01) > 0;
}

bool EntryUtil::IsExclude(const EntryId id) {
  return !IsInclude(id);
}

uint64_t EntryUtil::GetConjunctionId(const EntryId id) {
  return id >> 4;
}

int64_t EntryUtil::GetDocID(const EntryId id) {
  return ConjUtil::GetDocumentID(GetConjunctionId(id));
}

size_t EntryUtil::GetConjSize(const EntryId id) {
  return ConjUtil::GetConjunctionSize(GetConjunctionId(id));
}

std::string EntryUtil::ToString(const EntryId id) {
  std::ostringstream oss;
  if (id == NULLENTRY) {
    oss << "null";
  } else {
    oss << GetDocID(id) << "|" << (id & 0x01);
  }
  return oss.str();
}

std::string EntryUtil::ToString(const Attr& attr) {
  std::ostringstream oss;
  oss << "[" << attr.first << "," << attr.second << "]";
  return oss.str();
}

}  // namespace component
