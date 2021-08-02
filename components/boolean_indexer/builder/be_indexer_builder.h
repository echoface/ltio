#ifndef _LT_COMPONENT_BOOLEAN_INDEXER_BUILDER_H_
#define _LT_COMPONENT_BOOLEAN_INDEXER_BUILDER_H_

#include <unordered_map>

#include "base/lt_micro.h"
#include "components/boolean_indexer/be_indexer.h"
#include "components/boolean_indexer/document.h"

namespace component {

class BeIndexerBuilder {
public:
  BeIndexerBuilder(){};

  void AddDocument(Document&& doc);

  RefBooleanIndexer BuildIndexer();

private:
  // doc_id => Document
  std::unordered_map<int32_t, Document> documents_;

  DISALLOW_COPY_AND_ASSIGN(BeIndexerBuilder);
};

}  // namespace component

#endif
