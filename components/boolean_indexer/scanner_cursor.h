#ifndef _LT_COMPONENT_BE_INDEXER_CURSOR_H_
#define _LT_COMPONENT_BE_INDEXER_CURSOR_H_

#include "id_generator.h"

#include <iostream>
#include <memory>
#include <sstream>

namespace component {

/* represent a posting list for one Assign */
class EntriesCursor {
public:
  EntriesCursor(const Attr&, const Entries&);

  bool ReachEnd() const { return size_ <= index_; }

  EntryId GetCurEntryID() const {
    return size_ > index_ ? id_list_[index_] : NULLENTRY;
  }

  EntryId LinearSkipTo(EntryId id) {
    while (index_ < size_ && id_list_[index_] < id) {
      index_++;
    }
    return GetCurEntryID();
  }

  EntryId LinearSkip(EntryId id) {
    while (index_ < size_ && id_list_[index_] <= id) {
      index_++;
    }
    return GetCurEntryID();
  }

  EntryId Skip(const EntryId id);

  EntryId SkipTo(const EntryId id);

  size_t Size() const { return size_; }

  void DumpEntries(std::ostringstream& oss) const;

private:
  Attr attr_;  // eg: <age, 15>

  int index_ = 0;  // current cur index

  const size_t size_;

  const Entries& id_list_;  // [conj1, conj2, conj3]
};

class FieldCursor {
public:
  FieldCursor(){};

  void Initialize();

  void AddEntries(const Attr& attr, const Entries* entrylist);

  EntryId GetCurEntryID() const { return current_id_; }

  uint64_t GetCurConjID() const;

  size_t Size() const { return cursors_.size(); }

  EntryId Skip(const EntryId id);

  EntryId SkipTo(const EntryId id);

  void DumpEntries(std::ostringstream& oss) const;

private:
  EntryId current_id_ = 0;
  // eg: assign {"tag", "in", [1, 2, 3]}
  // out:
  // tag_2: [ID5]
  // tag_1: [ID1, ID2, ID7]
  // tag_3: null
  EntriesCursor* current_ = nullptr;

  std::vector<EntriesCursor> cursors_;
};
using FieldCursorPtr = std::unique_ptr<FieldCursor>;

// std::ostream& operator<< (std::ostream& os, const EntriesIterator&);
// std::ostream& operator<< (std::ostream& os, const FieldEntriesIter&);

}  // namespace component

#endif
