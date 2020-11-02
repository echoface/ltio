#ifndef _LT_COMPONENT_BOOLEAN_INDEXER_H_
#define _LT_COMPONENT_BOOLEAN_INDEXER_H_

#include <cassert>
#include <unordered_map>

#include "document.h"

namespace component {

#define NULLENTRY 0xFFFFFFFFFFFFFFFF

typedef uint64_t EntryId;
typedef std::vector<EntryId> EntryIdList;
typedef std::pair<std::string, int> Attr;

struct PostingList {
  Attr attr;            // eg: <age, 15>
  EntryIdList* id_list; // [conj1, conj2, conj3]
};

class PostingListGroup {
public:
  EntryId Skip(const EntryId id);
  EntryId SkipTo(const EntryId id);
private:
  //eg: assign {"user_tag", "in", [1, 2, 3]}
  //include all matched entry list for each Attr
  PostingList* current_;
  std::vector<PostingList> p_lists_;
};

class KSizeIndexes {
public:
  KSizeIndexes(size_t k_size);
  // make sure ids is sorted
  void AddIndexField(const Attr attr, EntryIdList ids) {
    index_map_[attr] = ids;
  }

  const EntryIdList* GetEntryList(const Attr& attr) const {
    auto iter = index_map_.find(attr);
    if (iter == index_map_.end()) {
      return nullptr;
    }
    return &(iter->second);
  }
private:
  const size_t k_size_;
  // Attr=>EntryIdList eg: (age, 15) => [1, 2, 5, 9]
  std::unordered_map<Attr, EntryIdList> index_map_;
};

class BooleanIndexer {
  public:
    BooleanIndexer(const size_t max_ksize);
    ~BooleanIndexer();

    const KSizeIndexes& GetIndexes(size_t k) {
      assert(k <=  max_ksize_);
      return ksize_indexes_[k];
    };

  private:
    const size_t max_ksize_;
    std::vector<KSizeIndexes> ksize_indexes_;
};

}
#endif


