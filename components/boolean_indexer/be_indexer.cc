#include "be_indexer.h"
#include "index_scanner.h"

namespace component {

namespace {
  static std::string wildcard_field("__z__");
  static Attr wildcard_attr(wildcard_field, 0);
}
const Attr& BooleanIndexer::WildcardAttr() {
  return wildcard_attr;
}

BooleanIndexer::BooleanIndexer()
  : wildcard_list_(nullptr){
}

BooleanIndexer::~BooleanIndexer() {
}

const KSizePostingEntries* BooleanIndexer::GetPostEntries(size_t k) const {
  if (k >= ksize_entries_.size()) {
    return nullptr;
  }
  return &(ksize_entries_[k]);
}

KSizePostingEntries* BooleanIndexer::MutableIndexes(size_t k) {
  while(ksize_entries_.size() < k + 1) {
    ksize_entries_.resize(k + 1);
  }
  return &ksize_entries_[k];
}

void BooleanIndexer::CompleteIndex() {
  for (KSizePostingEntries& pl : ksize_entries_) {
    pl.MakeEntriesSorted();
  }
  const auto zero_pl = GetPostEntries(0);
  wildcard_list_ = zero_pl->GetEntryList(wildcard_attr);
}

void BooleanIndexer::DumpIndex(std::ostringstream& oss) const {
  size_t size = 0;
  for (const KSizePostingEntries& pl : ksize_entries_) {
    oss << ">>>>>>> K:" << size++ << " index >>>>>>>>>>\n";
    pl.DumpPostingEntries(oss);
    oss << "\n";
  }
}

void BooleanIndexer::DumpIDMapping(std::ostringstream& oss) const {
  oss << "{\n";
  for (auto& kv : id_gen_) {
    oss << kv.first << " ==> " << kv.second << ",\n";
  }
  oss << "}\n";
}

uint64_t BooleanIndexer::GenUniqueID(const std::string& value) {
  auto iter = id_gen_.find(value);
  if (iter == id_gen_.end()) {
    /* bug here: why
    id_gen_[value] = (id_gen_.size() + 1);
    return id_gen_.size();
    */
    uint64_t value_id = id_gen_.size() + 1;
    id_gen_[value] = value_id;
    return value_id;
  }
  return iter->second;
}

std::vector<FieldCursorPtr>
BooleanIndexer::BuildFieldIterators(const size_t k_size,
                                    const FieldQueryValues &assigns) const {

  std::vector<FieldCursorPtr> result;

  if (k_size == 0 && wildcard_list_ && wildcard_list_->size()) {
    FieldCursorPtr field_iter(new FieldCursor());
    field_iter->AddEntries(wildcard_attr, wildcard_list_);
    result.emplace_back(std::move(field_iter));
  }

  assert(k_size <= ksize_entries_.size());
  const KSizePostingEntries* pl = GetPostEntries(k_size);

  for (const auto& field_attrs : assigns) {

    FieldCursorPtr field_iter(new FieldCursor());

    for (const ValueID& id : field_attrs.second) {
      Attr attr(field_attrs.first, id);
      field_iter->AddEntries(attr, pl->GetEntryList(attr));
    }

    if (field_iter->Size() > 0) {
      result.emplace_back(std::move(field_iter));
    }
  }
  for (FieldCursorPtr& iter : result) {
    iter->Initialize();
  }
  return result;
}

FieldQueryValues BooleanIndexer::ParseAssigns(const QueryAssigns& queries) const {
  FieldQueryValues assigns;

  for (const auto& assign : queries) {

    ValueList ids;
    for (const std::string &value : assign.Values()) {
      // parse value to id
      const auto& iter = id_gen_.find(value);
      if (iter == id_gen_.end()) {
        continue;
      }
      ids.push_back(iter->second);
    }
    if (ids.size()) {
      assigns[assign.name()] = ids;
    }
  }
  return assigns;
}

}
