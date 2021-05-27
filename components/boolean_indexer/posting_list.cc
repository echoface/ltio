#include "posting_list.h"

#include <algorithm>

namespace component {

bool EntriesIterator::ReachEnd() const {
  return id_list == nullptr || id_list->size() <= index;
}
EntryId EntriesIterator::GetCurEntryID() const {
  if (ReachEnd()) return NULLENTRY;
  return id_list->at(index);
}

EntryId EntriesIterator::Skip(const EntryId id) {
  EntryId cur_id = GetCurEntryID();
  if (cur_id > id) {
    return cur_id;
  }

  int tmp_idx = index;
  int right_idx = id_list->size();
  int mid;
  while(tmp_idx < right_idx) {
    mid = (tmp_idx + right_idx) >> 1;
    EntryId t_id = id_list->at(mid);
    if (t_id <= id) {
      tmp_idx = mid + 1;
    } else {
      right_idx = mid;
    }
  }
  index = tmp_idx;
  return ReachEnd() ? NULLENTRY : id_list->at(index);
}

EntryId EntriesIterator::SkipTo(const EntryId id) {
  EntryId cur_id = GetCurEntryID();
  if (cur_id > id) {
    return cur_id;
  }

  int tmp_idx = index;
  int right_idx = id_list->size();
  int mid;
  while(tmp_idx < right_idx) {
    mid = (tmp_idx + right_idx) >> 1;
    EntryId t_id = id_list->at(mid);
    if (t_id < id) {
      tmp_idx = mid + 1;
    } else {
      right_idx = mid;
    }
  }
  index = tmp_idx;
  return ReachEnd() ? NULLENTRY : id_list->at(index);
}

void PostingListGroup::Initialize() {
  /*
  std::sort(p_lists_.begin(), p_lists_.end(),
            [](const PostingList& a, const PostingList& b)-> bool {
              return a.GetCurEntryID() > b.GetCurEntryID();
            });
  */
  current_ = &p_lists_[0];
  EntryId min_id = NULLENTRY;

  for (auto& pl : p_lists_) {
    EntryId id = pl.GetCurEntryID();
    if (id < min_id) {
      min_id = id;
      current_ = &pl;
    }
  }
}

EntryId PostingListGroup::Skip(const EntryId id) {
  EntryId min_id = NULLENTRY;
  for (int i = 0; i < p_lists_.size(); i++) {
    EntryId id = p_lists_[i].SkipTo(id);
    if (id < min_id) {
      min_id = id;
      current_ = &p_lists_[i];
    }
  }
  return min_id;
}

EntryId PostingListGroup::SkipTo(const EntryId id) {

  EntryId min_id = NULLENTRY;
  for (int i = 0; i < p_lists_.size(); i++) {
    EntryId id = p_lists_[i].SkipTo(id);
    if (id < min_id) {
      min_id = id;
      current_ = &p_lists_[i];
    }
  }
  return min_id;
}


}
