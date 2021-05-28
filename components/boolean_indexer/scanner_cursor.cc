#include "scanner_cursor.h"

namespace component {


EntriesCursor::EntriesCursor(const Attr &a, const Entries *ids)
      : attr_(a), index_(0), id_list_(ids) {}

bool EntriesCursor::ReachEnd() const {
  return id_list_ == nullptr || id_list_->size() <= index_;
}

EntryId EntriesCursor::GetCurEntryID() const {
  if (ReachEnd()) return NULLENTRY;
  return id_list_->at(index_);
}

EntryId EntriesCursor::Skip(const EntryId id) {
  EntryId cur_id = GetCurEntryID();
  if (cur_id > id) {
    return cur_id;
  }

  size_t size = id_list_->size();
  int right_idx = size;
  int mid;
  while(index_ < right_idx) {
    mid = (index_ + right_idx) >> 1;

    if ((*id_list_)[mid] <= id) {
      index_ = mid + 1;
    } else {
      right_idx = mid;
    }
    if (index_ >= size || (*id_list_)[index_] > id) {
      break;
    }
  }
  return GetCurEntryID();
}

EntryId EntriesCursor::SkipTo(const EntryId id) {
  EntryId cur_id = GetCurEntryID();
  if (cur_id >= id) {
    return cur_id;
  }

  size_t size = id_list_->size();
  int right_idx = size;

  int mid;
  while(index_ < right_idx) {
    mid = (index_ + right_idx) >> 1;
    if ((*id_list_)[mid] >= id) {
      right_idx = mid;
    } else {
      index_ = mid + 1;
    }
    if (index_ >= size || (*id_list_)[index_] >= id) {
      break;
    }
  }
  return GetCurEntryID();
}

void FieldCursor::AddEntries(const Attr& attr, const Entries* entrylist) {
  if (nullptr == entrylist) {
    return;
  }
  cursors_.emplace_back(attr, entrylist);
  current_ = &cursors_.front();
}

void FieldCursor::Initialize() {
  current_ = &cursors_[0];
  EntryId min_id = NULLENTRY;

  for (auto& pl : cursors_) {
    EntryId id = pl.GetCurEntryID();
    if (id < min_id) {
      min_id = id;
      current_ = &pl;
    }
  }
}

EntryId FieldCursor::GetCurEntryID() const {
    return current_->GetCurEntryID();
}

uint64_t FieldCursor::GetCurConjID() const {
  return EntryUtil::GetConjunctionId(current_->GetCurEntryID());
}

EntryId FieldCursor::Skip(const EntryId id) {
  EntryId min_id = NULLENTRY;
  for (int i = 0; i < cursors_.size(); i++) {
    EntryId eid = cursors_[i].SkipTo(id);
    if (eid < min_id) {
      min_id = eid;
      current_ = &cursors_[i];
    }
  }
  return min_id;
}

EntryId FieldCursor::SkipTo(const EntryId id) {
  uint64_t cur = current_->GetCurEntryID();
  EntryId min_id = NULLENTRY;
  for (int i = 0; i < cursors_.size(); i++) {
    EntryId eid = cursors_[i].SkipTo(id);
    if (eid < min_id) {
      min_id = eid;
      current_ = &cursors_[i];
    }
  }
  return min_id;
}

void EntriesCursor::DumpEntries(std::ostringstream& oss) const {
  oss << "atrr:" << EntryUtil::ToString(attr_)
    << ", cur:" << EntryUtil::ToString(GetCurEntryID())
    << ", eids:[";
  for (const auto& id : *id_list_) {
    oss << EntryUtil::ToString(id) << "," ;
  }
  oss << "]\n";
}

void FieldCursor::DumpEntries(std::ostringstream& oss) const {
  for (auto& cursor: cursors_) {
    if ((&cursor) == current_) {
      oss << ">";
    }
    cursor.DumpEntries(oss);
  }
}

}//end component
