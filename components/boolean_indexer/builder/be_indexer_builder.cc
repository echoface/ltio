#include "be_indexer_builder.h"

#include <algorithm>

#include <glog/logging.h>

#include "components/boolean_indexer/id_generator.h"
#include "fmt/format.h"

namespace component {

BeIndexerBuilder::BeIndexerBuilder() {
  indexer_ = RefBooleanIndexer(new BooleanIndexer());
  // default_parser_ = std::make_shared<NumberParser>();
  default_parser_ = std::make_shared<HasherParser>();
}

FieldMeta* BeIndexerBuilder::GetFieldMeta(const std::string& field) {
  auto meta = indexer_->GetMeta(field);
  if (meta != nullptr) {
    return meta;
  }

  FieldMetaPtr meta_ptr(new FieldMeta());
  meta_ptr->name = field;
  meta_ptr->parser = default_parser_;

  meta = meta_ptr.get();
  indexer_->SetMeta(field, std::move(meta_ptr));
  return meta;
}

void BeIndexerBuilder::AddDocument(Document&& doc) {
  for (Conjunction* conj : doc.conjunctions()) {
    uint64_t conj_id = conj->id();

    if (conj->size() == 0) {
      indexer_->AddWildcardEntry(conj->id());
    }

    for (const auto& field_expr : conj->ExpressionAssigns()) {
      const BooleanExpr& expr = field_expr.second;
      const std::string& field = field_expr.first;

      EntryId eid =
          EntryUtil::GenEntryID(conj->id(), field_expr.second.exclude());

      FieldMeta* meta = GetFieldMeta(field);
      EntriesContainer* container = indexer_->GetContainer(field);

      TokenPtr token = meta->parser->ParseIndexing(expr.Values());
      if (token->BadToken()) {
        LOG(ERROR) << "expression can't be parsed, field:" << field
                   << ", values:"
                   << fmt::format("{}", fmt::join(expr.Values(), ","));
        return;
      }
      container->IndexingToken(meta, eid, token.get());
    }
  }
}

RefBooleanIndexer BeIndexerBuilder::BuildIndexer() {
  bool ok = indexer_->CompleteIndex();
  if (!ok) {
    return nullptr;
  }
  return indexer_;
}

}  // namespace component
