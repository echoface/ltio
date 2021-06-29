#include "be_indexer_builder.h"

#include <algorithm>

#include "components/boolean_indexer/id_generator.h"

namespace component {

void BeIndexerBuilder::AddDocument(Document&& doc) {
  auto result = documents_.insert(std::make_pair(doc.doc_id(), std::move(doc)));

  if (!result.second) {
    return;
  }
}

RefBooleanIndexer BeIndexerBuilder::BuildIndexer() {
  RefBooleanIndexer indexer(new BooleanIndexer());

  for (const auto& doc : documents_) {
    for (Conjunction* conj : doc.second.conjunctions()) {
      uint64_t conj_id = conj->id();

      auto kindexes = indexer->MutableIndexes(conj->size());

      if (conj->size() == 0) {
        auto& wc_attr = BooleanIndexer::WildcardAttr();
        kindexes->AddEntry(wc_attr, EntryUtil::GenEntryID(conj->id(), false));
      }

      for (const auto& name_expr_kv : conj->ExpressionAssigns()) {
        const std::string& assin_name = name_expr_kv.first;

        const BooleanExpr& expr = name_expr_kv.second;

        // TODO: to do parser ExpressionsValue to values
        for (const auto& value : expr.Values()) {
          Attr attr(assin_name, indexer->GenUniqueID(value));
          kindexes->AddEntry(attr,
                             EntryUtil::GenEntryID(conj->id(), expr.exclude()));
        }
      }
    }
  }

  indexer->CompleteIndex();

  return indexer;
}

}  // namespace component
