#ifndef _LT_COMPONENT_BOOLEAN_INDEXER_H_
#define _LT_COMPONENT_BOOLEAN_INDEXER_H_

#include <algorithm>
#include <cassert>
#include <set>
#include <sstream>
#include <unordered_map>

#include "document.h"
#include "id_generator.h"
#include "posting_list.h"

namespace component {


struct pair_hash {
	template <class T1, class T2>
	std::size_t operator() (const std::pair<T1, T2> &pair) const {
		return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
	}
};
typedef std::unordered_map<Attr, EntryIdList, pair_hash> IndexMap;

class KSizeIndexes {
public:
  KSizeIndexes() {}

  void MakeIndexSorted();
  void AddEntry(const Attr& attr, EntryId id);

  const EntryIdList* GetEntryList(const Attr& attr) const;

  void DumpAttrEntries(std::ostringstream& oss);
private:
  // Attr=>EntryIdList
  // eg: (age, 15) => [1, 2, 5, 9]
  IndexMap index_map_;
};

class BooleanIndexer {
  public:
    typedef std::vector<Assigns> QueryAssigns;

    BooleanIndexer(const size_t max_ksize);
    ~BooleanIndexer();

    const KSizeIndexes& GetIndexes(size_t k) {
      assert(k <=  max_ksize_);
      return ksize_indexes_[k];
    };

    void CompleteIndex();

    KSizeIndexes* MutableIndexes(size_t k);

    size_t GetPostingLists(const size_t k_size,
                           const QueryAssigns& quries,
                           std::vector<PostingListGroup>* out);

    void InitPostingListGroup(std::vector<PostingListGroup>& plists);

    int Query(const QueryAssigns& quries,
              std::set<int32_t>* result,
              const bool dump_details = false);

    void DumpIndex();

    uint64_t GetUniqueID(const std::string& value);
  private:

    const size_t max_ksize_;

    std::unordered_map<std::string, uint64_t> id_gen_;

    std::vector<KSizeIndexes> ksize_indexes_;
    const EntryIdList* wildcard_list_;
};

}
#endif


