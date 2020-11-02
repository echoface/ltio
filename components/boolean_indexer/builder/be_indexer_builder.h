#ifndef _LT_COMPONENT_BOOLEAN_INDEXER_BUILDER_H_
#define _LT_COMPONENT_BOOLEAN_INDEXER_BUILDER_H_

#include <unordered_map>

#include "base/base_micro.h"
#include "components/boolean_indexer/be_indexer.h"
#include "components/boolean_indexer/document.h"

namespace component {

typedef std::unique_ptr<BooleanIndexer> BooleanIndexerPtr;

class BeIndexerBuilder {
  public:
    BeIndexerBuilder(){};

    void AddDocument(Document&& doc);

    BooleanIndexerPtr BuildIndexer();
  private:
    size_t max_conj_size_ = 0;

    // doc_id => Document
    std::unordered_map<int32_t, Document> documents_;

    // conj_id => Conjunction
    std::unordered_map<uint64_t, Conjunction*> conjunctions_;

    DISALLOW_COPY_AND_ASSIGN(BeIndexerBuilder);
};

}

#endif
