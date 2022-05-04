#include "be_indexer.h"
#include "index_scanner.h"

#include "glog/logging.h"
#include "common_container.h"

namespace component {

namespace {
static std::string wildcard_field("__z__");
static Attr wildcard_attr(wildcard_field, 0);

static const std::string default_field_key = "___default___";
}  // namespace

BooleanIndexer::BooleanIndexer() {
  containers_[default_field_key] = EntriesContainerPtr(new CommonContainer());
}

BooleanIndexer::~BooleanIndexer() {}

void BooleanIndexer::AddWildcardEntry(uint64_t conj_id) {
  EntryId eid = EntryUtil::GenEntryID(conj_id, false);
  wildcard_list_.push_back(eid);
}

bool BooleanIndexer::CompleteIndex() {
  if (!wildcard_list_.empty()) {
    std::sort(wildcard_list_.begin(), wildcard_list_.end());
  }
  for (auto& field_container : containers_) {
    bool ok = field_container.second->CompileEntries();
    if (!ok) {
      LOG(ERROR) << "compile index fail:" << field_container.first;
      return false;
    }
  }
  return true;
}

void BooleanIndexer::DumpIndex(std::ostringstream& oss) const {
  size_t size = 0;
  for (auto& field_container : containers_) {
    const std::string& field = field_container.first;
    const EntriesContainer* container = field_container.second.get();
  }
}

FieldCursorPtr BooleanIndexer::GenWildcardCursor() const {
  if (wildcard_list_.empty()) {
    return nullptr;
  }

  FieldCursorPtr wc_cursor(new FieldCursor());
  wc_cursor->AddEntries(wildcard_attr, &wildcard_list_);
  return wc_cursor;
}

FieldMeta* BooleanIndexer::GetMeta(const std::string& field) const {
  auto iter = field_meta_.find(field);
  if (iter == field_meta_.end()) {
    return nullptr;
  }
  return iter->second.get();
}

bool BooleanIndexer::SetMeta(const std::string& field, FieldMetaPtr meta) {
  auto it = field_meta_.insert(std::make_pair(field, std::move(meta)));
  LOG_IF(ERROR, !it.second) << "configure field meta fail, field:" << field;
  return it.second;
}

EntriesContainer* BooleanIndexer::GetContainer(const std::string field) const {
  auto iter = containers_.find(field);
  if (iter != containers_.end()) {
    return iter->second.get();
  }
  iter = containers_.find(default_field_key);
  return iter->second.get();
}

}  // namespace component
