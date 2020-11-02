#ifndef _LT_COMPONENT_BOOLEAN_INDEXER_IDGEN_H_
#define _LT_COMPONENT_BOOLEAN_INDEXER_IDGEN_H_

#include <cstdint>

namespace component {

struct ConjunctionIdDesc {
  ConjunctionIdDesc(uint64_t conjunction_id);
  uint32_t doc_id;
  uint8_t  idx_in_doc;
  uint8_t  conj_size;
};

//doc: docment id
//idx: index of this conjunction in owned document
//size: conjunction size (the count of include type booleanexpression)
// |--empty(8)--|--empty(8)--|--doc(32)--|--index(8)--|--size(8)--|
uint64_t GenConjunctionId(uint32_t doc, uint8_t index, uint8_t size);


// conj_id: conjunction_id
// |--doc(32)--|--index(8)--|--size(8)--|--empty(8)--|--empty(7)--|--exclude(1)--|
uint64_t GenPostingListEntryId(uint64_t conj_id, bool exclude);

}
#endif
