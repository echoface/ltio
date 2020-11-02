#include "be_indexer.h"

namespace component {

BooleanIndexer::BooleanIndexer(const size_t max_ksize)
  : max_ksize_(max_ksize),
    ksize_indexes_(max_ksize + 1) {

}

}
