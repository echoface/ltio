#ifndef _LT_COMPONENT_BOOLEAN_INDEXER_BUILDER_H_
#define _LT_COMPONENT_BOOLEAN_INDEXER_BUILDER_H_

#include <unordered_map>

#include "base/lt_micro.h"
#include "components/boolean_indexer/be_indexer.h"
#include "components/boolean_indexer/document.h"
#include "components/boolean_indexer/parser/number_parser.h"

namespace component {

class BeIndexerBuilder {
public:
  BeIndexerBuilder();

  FieldMeta* GetFieldMeta(const std::string& field);

  void AddDocument(Document&& doc);

  RefBooleanIndexer BuildIndexer();

private:
  RefBooleanIndexer indexer_;

  RefExprParser default_parser_;

  DISALLOW_COPY_AND_ASSIGN(BeIndexerBuilder);
};

}  // namespace component

#endif
