#include "index_scanner.h"

namespace component {

IndexScanner::IndexScanner(BooleanIndexer* index)
    : index_(index) {
}

std::set<uint32_t> IndexScanner::Retrieve(QueryAssigns queries, Option opt) {
    return {};
}


}