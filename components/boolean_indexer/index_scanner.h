#ifndef _LT_COMPONENT_BE_INDEXER_H_
#define _LT_COMPONENT_BE_INDEXER_H_

#include "be_indexer.h"

namespace component {

class IndexScanner {
public:
    struct Option {
        bool dump_detail;
    };
    IndexScanner(BooleanIndexer* index);

    std::set<uint32_t> Retrieve(QueryAssigns queries, Option opt);
private:
    BooleanIndexer* index_;
};
}


#endif