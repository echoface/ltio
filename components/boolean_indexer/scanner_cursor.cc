#include "scanner_cursor.h"

namespace component {
namespace {
  static const int linear_skip_distance = 64;
}

EntriesCursor::EntriesCursor(const Attr& a, const Entries& ids)
    : attr_(a), index_(0), size_(ids.size()), id_list_(ids) {}

EntryId EntriesCursor::Skip(const EntryId id) {
  int right_idx = size_;
  int mid;
  while (index_ < right_idx) {
    if (right_idx - index_ < linear_skip_distance) {
      return LinearSkip(id);
    }
    mid = (index_ + right_idx) >> 1;

    if (id_list_[mid] <= id) {
      index_ = mid + 1;
    } else {
      right_idx = mid;
    }
    if (index_ >= size_ || id_list_[index_] > id) {
      break;
    }
  }
  return GetCurEntryID();
}

EntryId EntriesCursor::SkipTo(const EntryId id) {
  int right_idx = size_;

  int mid;
  while(index_ < right_idx) {
    if (right_idx - index_ < linear_skip_distance) {
      return LinearSkipTo(id);
    }
    mid = (index_ + right_idx) >> 1;
    if ((id_list_)[mid] >= id) {
      right_idx = mid;
    } else {
      index_ = mid + 1;
    }
    if (index_ >= size_ || (id_list_)[index_] >= id) {
      break;
    }
  }
  return GetCurEntryID();
}

void FieldCursor::AddEntries(const Attr& attr, const Entries* entrylist) {
  if (nullptr == entrylist) {
    return;
  }
  cursors_.emplace_back(attr, *entrylist);
}

void FieldCursor::Initialize() {
  current_ = &cursors_[0];
  current_id_ = current_->GetCurEntryID();
  for (auto& pl : cursors_) {
    EntryId id = pl.GetCurEntryID();
    if (id < current_id_) {
      current_ = &pl;
      current_id_ = id;
    }
  }
}

uint64_t FieldCursor::GetCurConjID() const {
  return EntryUtil::GetConjunctionId(current_id_);
}

EntryId FieldCursor::Skip(const EntryId id) {
  EntryId min_id = NULLENTRY;
  for (auto& cursor : cursors_) {
    EntryId eid = cursor.Skip(id);
    if (eid < min_id) {
      min_id = eid;
      current_ = &cursor;
    }
  }
  return current_id_ = current_->GetCurEntryID();
}

EntryId FieldCursor::SkipTo(const EntryId id) {
  EntryId min_id = NULLENTRY;
  for (auto& cursor : cursors_) {
    EntryId eid = cursor.SkipTo(id);
    if (eid < min_id) {
      min_id = eid;
      current_ = &cursor;
    }
  }
  return current_id_ = current_->GetCurEntryID();
}

void EntriesCursor::DumpEntries(std::ostringstream& oss) const {
  oss << "atrr:" << EntryUtil::ToString(attr_)
    << ", cur:" << EntryUtil::ToString(GetCurEntryID())
    << ", eids:[";
  for (const EntryId id : id_list_) {
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
