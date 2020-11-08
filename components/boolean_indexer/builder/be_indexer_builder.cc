#include "be_indexer_builder.h"

#include <algorithm>

#include "components/boolean_indexer/id_generator.h"

namespace component {

void BeIndexerBuilder::AddDocument(Document&& doc) {

  auto result = documents_.insert(
    std::make_pair(doc.doc_id(), std::move(doc)));

  if (!result.second) {
    return;
  }

  Document* doc_t = &result.first->second;
  for (auto& conj : doc_t->conjunctions()) {
    conjunctions_[conj->id()] = conj;
    max_conj_size_ = std::max(conj->size(), max_conj_size_);
  }
}

BooleanIndexerPtr BeIndexerBuilder::BuildIndexer() {
  BooleanIndexerPtr indexer(new BooleanIndexer(max_conj_size_));

  for (auto conj_kv : conjunctions_) {

    uint64_t conj_id = conj_kv.first;
    Conjunction* conj = conj_kv.second;

    KSizeIndexes* kindexes = indexer->MutableIndexes(conj->size());

    if (conj->size() == 0) {
      Attr attr("__wildcard__", indexer->GetUniqueID("__wildcard__"));
      kindexes->AddEntry(attr, EntryUtil::GenEntryID(conj->id(), false));
    }

    for (const auto& name_expr_kv : conj->ExpressionAssigns()) {
      const std::string& assin_name = name_expr_kv.first;

      const BooleanExpr& expr = name_expr_kv.second;

      //TODO: to do parser ExpressionsValue to values
      for (const auto& value : expr.Values()) {
        Attr attr(assin_name, indexer->GetUniqueID(value));
        kindexes->AddEntry(attr, EntryUtil::GenEntryID(conj->id(), expr.exclude()));
      }
    }
  }

  indexer->CompleteIndex();

  indexer->DumpIndex();
  return indexer;
}


}
