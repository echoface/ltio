#include "id_generator.h"

namespace component {

// |--doc(32)--|--index(8)--|--size(8)--|--- empty(8) ---|---empty(8) ---- |
uint64_t GenConjunctionId(uint32_t doc, uint8_t idx, uint8_t size) {
  return (uint64_t(doc) << 16) | (uint64_t(idx) << 8) | uint64_t(size);
}

ConjunctionIdDesc::ConjunctionIdDesc(uint64_t conjunction_id) 
  : doc_id((conjunction_id >> 16) & 0xFFFFFFFF),
    idx_in_doc((conjunction_id >> 8) & 0xFF),
    conj_size(conjunction_id & 0xFF) {
}

uint64_t GenPostingListEntryId(uint64_t conj_id, bool exclude) {
  conj_id = conj_id << 16;
  return exclude ? conj_id : conj_id & 0x01;
}

}
