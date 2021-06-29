//
// Created by gh on 18-10-15.
//

#ifndef LIGHTINGIO_POSTING_LIST_MANAGER_H
#define LIGHTINGIO_POSTING_LIST_MANAGER_H

#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include "bitmap_posting_list.h"
#include "glog/logging.h"

namespace component {

class PostingListManager {
public:
  void SetSortedIndexedIds(const std::vector<int64_t>&& ids);
  BitMapPostingList* BuildBitMapPostingListForIds(const std::set<int64_t>& ids);

  BitMapPlPtr CreateMergePostingList();
  std::set<int64_t> GetIdsFromPostingList(const BitMapPostingList* pl);

private:
  // vector's hashvalue to vector
  std::vector<int64_t> sorted_doc_ids_;
  std::unordered_map<int64_t, uint64_t> doc_id_to_bit_idx_;
  std::unordered_map<uint64_t, int64_t> bit_idx_to_doc_id_;

  std::unordered_map<uint64_t, BitMapPlPtr> ids_table_;
};

}  // namespace component
#endif  // LIGHTINGIO_POSTING_LIST_MANAGER_H
