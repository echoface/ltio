#ifndef _LT_COMPONENT_BE_INDEXER_H_
#define _LT_COMPONENT_BE_INDEXER_H_

#include "be_indexer.h"
#include <vector>

namespace component {

class IndexScanner {
public:
    struct Option {
        bool dump_detail;
    };
    struct Result {
        std::string to_string() const;
        int error_code;
        std::vector<uint32_t> result;
    };
    IndexScanner(BooleanIndexer* index);

    static const Option& DefaultOption();

    Result Retrieve(const QueryAssigns& queries,
                    const Option* opt = nullptr) const;
private:
    BooleanIndexer* index_;
};

}


#endif
