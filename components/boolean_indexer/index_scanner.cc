#include "index_scanner.h"
#include <unistd.h>
#include <vector>

namespace component {

static const IndexScanner::Option default_option = {
  .dump_detail = false
};

std::string IndexScanner::Result::to_string() const {
  std::ostringstream oss;
  oss << "code:" << error_code << ",id:[";
  for (auto id : result) {
    oss << id << ",";
  }
  oss << "]";
  return oss.str();
}


IndexScanner::IndexScanner(BooleanIndexer* index)
    : index_(index) {
}

const IndexScanner::Option& IndexScanner::DefaultOption() {
  return  default_option;
}

IndexScanner::Result
IndexScanner::Retrieve(const QueryAssigns& queries, const Option* opt) const{
  if (nullptr == opt) {
    opt = &default_option;
  }

  FieldQueryValues field_values = index_->ParseAssigns(queries);

  IndexScanner::Result result;

  int k = std::min(index_->MaxKSize(), field_values.size());
  for (; k >= 0; k--) {

    std::vector<FieldCursorPtr> plists
      = index_->BuildFieldIterators(k, field_values);

    int temp_k = k;
    if (temp_k == 0) {
      temp_k = 1;
    }

    if (opt->dump_detail) {
      std::ostringstream oss;
      oss << "\nretrive k:" << k << " >>>>>\n";
      for (auto& c : plists) {
        c->DumpEntries(oss);
      }
      std::cout << oss.str();
    }

    if (plists.size() < temp_k) {
      continue;
    }

    std::sort(plists.begin(), plists.end(),
              [](FieldCursorPtr& l, FieldCursorPtr& r) -> bool {
                return l->GetCurEntryID() < r->GetCurEntryID();
              });
    EntryId end_eid = plists[temp_k - 1]->GetCurEntryID();

    while (end_eid != NULLENTRY) {

      EntryId eid = plists[0]->GetCurEntryID();
      uint64_t conj_id = EntryUtil::GetConjunctionId(eid);
      EntryId next_id =
        EntryUtil::IsInclude(end_eid) ? end_eid - 1 : end_eid;

      if (opt->dump_detail) {
        std::ostringstream oss;
        oss << "[" << EntryUtil::ToString(eid) << "..."
          << EntryUtil::ToString(end_eid) << "]\ncursors:[";
        for (auto& c : plists) {
          oss << EntryUtil::ToString(c->GetCurEntryID()) << ",";
        }
        oss << "]\n";
        std::cout << oss.str();
      }

      if (conj_id == EntryUtil::GetConjunctionId(end_eid)) {

        next_id = end_eid + 1;

        if (EntryUtil::IsInclude(eid)) {

          result.result.push_back(EntryUtil::GetDocID(eid));

        } else { //exclude
          for (auto& iter : plists) {
            if (iter->GetCurConjID() != conj_id) {
              break;
            }
            iter->Skip(next_id);
          }
        } //end exclude
      }
      // 推进游标
      for (int i = 0; i < temp_k; i++) {
        plists[i]->SkipTo(next_id);
      }

      std::sort(plists.begin(), plists.end(),
                [](FieldCursorPtr& l, FieldCursorPtr& r) -> bool {
                  return l->GetCurEntryID() < r->GetCurEntryID();
                });
      end_eid = plists[temp_k - 1]->GetCurEntryID();
    }// end while

  }// to k = k-1
  return result;
}


}
