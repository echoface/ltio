#include "be_indexer_builder.h"

#include "components/boolean_indexer/id_generator.h"

namespace component {

void BeIndexerBuilder::AddDocument(Document&& doc) {
  auto result = documents_.insert(std::make_pair(doc.doc_id(), std::move(doc)));
  if (!result.second) {
    return;
  }
  Document* doc_t = &result.first->second;
  size_t idx = 0;
  for (auto& conj : doc_t->conjunctions()) {
    uint64_t conj_id =
      GenConjunctionId(doc_t->doc_id(), idx, conj->size());

    idx++;
    conj->SetConjunctionId(conj_id);

    conjunctions_[conj_id] = conj;
    max_conj_size_ = std::max(conj->size(), max_conj_size_);
  }
}

BooleanIndexerPtr BeIndexerBuilder::BuildIndexer() {
  BooleanIndexerPtr indexer(new BooleanIndexer(max_conj_size_));
  
  for (auto conj_kv : conjunctions_) {
    //conj_kv.second->
  }
  return indexer;
}


}
