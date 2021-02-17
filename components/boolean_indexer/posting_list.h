#ifndef _LT_COMPONENT_BOOLEAN_INDEX_PL_H_
#define _LT_COMPONENT_BOOLEAN_INDEX_PL_H_

#include "id_generator.h"

#include <sstream>

namespace component {

#define NULLENTRY 0xFFFFFFFFFFFFFFFF

/* represent a posting list for one Assign */
struct PostingList {
  PostingList(const Attr& a, int idx, const EntryIdList* ids)
    : attr(a),
      index(idx),
      id_list(ids) {
  }

  bool ReachEnd() const;
  EntryId GetCurEntryID() const;
  EntryId Skip(const EntryId id);
  EntryId SkipTo(const EntryId id);

  Attr attr;            // eg: <age, 15>
  int index;            // current cur index
  const EntryIdList* id_list; // [conj1, conj2, conj3]
};

class PostingListGroup {
public:
  PostingListGroup()
    : current_(nullptr) {
  }

  void AddPostingList(const Attr& attr, const EntryIdList* entrylist) {
    p_lists_.emplace_back(attr, 0, entrylist);
  }

  void Initialize();

  EntryId GetCurEntryID() const {
    return current_->GetCurEntryID();
  }
  bool IsCurEntryExclude() const {
    return EntryUtil::HasExclude(current_->GetCurEntryID());
  }
  uint64_t GetCurConjID() const {
    return EntryUtil::GetConjunctionId(current_->GetCurEntryID());
  }

  EntryId Skip(const EntryId id);

  EntryId SkipTo(const EntryId id);

  void DumpPostingList(std::ostringstream& oss) {
    for (auto& pl : p_lists_) {
      EntryId cur_id = pl.GetCurEntryID();
      oss << pl.attr.first << "#" << pl.attr.second
        << ", cur:" << cur_id
        << ", doc:" << ConjUtil::GetDocumentID(EntryUtil::GetConjunctionId(cur_id))
        << ":[";
      for (auto& id : *pl.id_list) {
        oss << ConjUtil::GetDocumentID(EntryUtil::GetConjunctionId(id))
          << ", ";
      }
      oss << "]\n";
    }
  }

  //eg: assign {"tag", "in", [1, 2, 3]}
  //out:
  // tag_2: [ID5]
  // tag_1: [ID1, ID2, ID7]
  // tag_3: null
  PostingList* current_;
  std::vector<PostingList> p_lists_;
};


}
#endif
