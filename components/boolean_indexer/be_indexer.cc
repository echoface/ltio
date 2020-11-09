#include "be_indexer.h"

namespace component {

void KSizeIndexes::AddEntry(const Attr& attr, EntryId id) {
  index_map_[attr].emplace_back(id);
}


void KSizeIndexes::MakeIndexSorted() {
  for (auto& attr_entries : index_map_) {
    EntryIdList& entries = attr_entries.second;
    std::sort(entries.begin(), entries.end());
  }
}

const EntryIdList* KSizeIndexes::GetEntryList(const Attr& attr) const {
  auto iter = index_map_.find(attr);
  if (iter == index_map_.end()) {
    return nullptr;
  }
  return &(iter->second);
}


void KSizeIndexes::DumpAttrEntries(std::ostringstream& oss) {
  for (auto& attr_entries : index_map_) {
    const Attr& attr = attr_entries.first;
    EntryIdList& entries = attr_entries.second;

    oss << attr.first << "#" << attr.second << ":[";
    bool is_first = true;
    for (auto& entry_id : entries) {
      uint64_t conj_id = EntryUtil::GetConjunctionId(entry_id);
      if (!is_first) {
        oss << ",";
      }
      oss << "("<< ConjUtil::GetDocumentID(conj_id)
        << "," << (EntryUtil::HasExclude(entry_id) ? "ex)" : "in)");
      is_first = false;
    }
    oss << "]\n";
  }
};

BooleanIndexer::BooleanIndexer(const size_t max_ksize)
  : max_ksize_(max_ksize),
    ksize_indexes_(max_ksize + 1) {
}

BooleanIndexer::~BooleanIndexer() {
}

KSizeIndexes* BooleanIndexer::MutableIndexes(size_t k) {
  assert(k <=  max_ksize_);
  return &ksize_indexes_[k];
}

void BooleanIndexer::CompleteIndex() {
  for (KSizeIndexes& indexes : ksize_indexes_) {
    indexes.MakeIndexSorted();
  }
  Attr attr("__wildcard__", GetUniqueID("__wildcard__"));
  const KSizeIndexes& zero_indexes = GetIndexes(0);
  const EntryIdList* wildcard_list = zero_indexes.GetEntryList(attr);
  wildcard_list_ = wildcard_list;
}

void BooleanIndexer::DumpIndex() {
  std::ostringstream oss;
  size_t size = 0;
  for (KSizeIndexes& indexes : ksize_indexes_) {
    oss << ">>>>>>> K:" << size++ << " index >>>>>>>>>>\n";
    indexes.DumpAttrEntries(oss);
    oss << "\n";
  }
  printf("%s", oss.str().data());
}

uint64_t BooleanIndexer::GetUniqueID(const std::string& value) {
  auto iter = id_gen_.find(value);
  if (iter == id_gen_.end()) {
    id_gen_[value] = id_gen_.size() + 1;
    return id_gen_.size();
  }
  return iter->second;
}

size_t BooleanIndexer::GetPostingLists(const size_t k_size,
                                       const QueryAssigns& quries,
                                       std::vector<PostingListGroup>* out) {

  if (k_size == 0 && wildcard_list_) {
    static const Attr wildcard_attr("__wildcard__", 0);
    PostingListGroup group;
    group.AddPostingList(wildcard_attr, wildcard_list_);
    out->emplace_back(group);
  }

  const KSizeIndexes& index_data = GetIndexes(k_size);
  for (const auto& assign : quries) {

    PostingListGroup group;

    for (const std::string& value : assign.Values()) {
      auto iter = id_gen_.find(value);
      if (iter == id_gen_.end()) {
        continue;
      }

      Attr attr(assign.name(), iter->second);
      const EntryIdList* entrylist =  index_data.GetEntryList(attr);
      if (!entrylist) {
        continue;
      }
      group.AddPostingList(attr, entrylist);
    }

    if (group.p_lists_.size()) {
      out->emplace_back(std::move(group));
    }
  }
  return out->size();
}

// init current and sort by order
void BooleanIndexer::InitPostingListGroup(std::vector<PostingListGroup>& plists) {
  for (auto& group : plists) {
    group.Initialize();
  }
}

int BooleanIndexer::Query(const QueryAssigns& quries,
                          std::set<int32_t>* result,
                          const bool dump_details) {

  int k = std::min(max_ksize_, quries.size());
  for (; k >= 0; k--) {

    std::vector<PostingListGroup> plists;

    size_t count = GetPostingLists(k, quries, &plists);
   
    if (dump_details) {
      std::ostringstream oss;
      oss << ">>>dump plist for k_size:" << k << ">>>>>\n";
      for (auto& grp : plists) {
        oss << "\t", grp.DumpPostingList(oss);
      }
      std::cout << oss.str();
    }

    int temp_k = k;
    if (temp_k == 0) {
      temp_k = 1;
    }

    if (count < temp_k) {
      continue;
    }

    InitPostingListGroup(plists);

    while(plists[temp_k - 1].GetCurEntryID() != NULLENTRY) {

      std::sort(plists.begin(),
                plists.end(),
                [](PostingListGroup& l, PostingListGroup& r) -> bool {
                  return l.GetCurEntryID() < r.GetCurEntryID();
                });

      if (dump_details) {
        std::ostringstream oss2;
        oss2 << "after sort plist group\n";
        for (auto& grp : plists) {
          oss2 << "\t", grp.DumpPostingList(oss2);
        }
        std::cout << oss2.str();
      }

      EntryId id = plists[0].GetCurEntryID();

      //skip to k-1's entry id
      if (plists[0].GetCurConjID() != plists[temp_k - 1].GetCurConjID()) {
        EntryId skip_id = plists[temp_k - 1].GetCurEntryID();
        for (int l = 0; l < temp_k; l++) {
          plists[l].SkipTo(skip_id);
        }
        continue;
      }

      EntryId next_id = id + 1;
      // entry[0] == entry[k-1]
      if (EntryUtil::HasExclude(id)) { //is exclude
        uint64_t rejected_id = plists[0].GetCurConjID();
        for (int l = temp_k; l < plists.size(); l++) {
          if (plists[l].GetCurConjID() != rejected_id) {
            break;
          }
          plists[l].Skip(id);
        }

      } else { //hit, push to result set

        uint64_t conj_id = EntryUtil::GetConjunctionId(id);
        result->insert(ConjUtil::GetDocumentID(conj_id));
      }

      for (int l = 0; l < temp_k; l++) {
        plists[l].SkipTo(next_id); //to next smallest entry id
      }
      //end handle of "include/exclude"

    }// end while

  }// to k = k-1
  return 0;
}


}
