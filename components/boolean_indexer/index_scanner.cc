#include "index_scanner.h"
#include <unistd.h>
#include <vector>

#include "glog/logging.h"

namespace component {

static const IndexScanner::Option default_option = {.dump_detail = false};

std::string IndexScanner::Result::to_string() const {
  std::ostringstream oss;
  oss << "code:" << error_code << ",id:[";
  for (auto id : result) {
    oss << id << ",";
  }
  oss << "]";
  return oss.str();
}

IndexScanner::IndexScanner(BooleanIndexer* index) : index_(index) {}

const IndexScanner::Option& IndexScanner::DefaultOption() {
  return default_option;
}

FieldCursors IndexScanner::init_cursors(const QueryAssigns& queries,
                                        const Option* opt) const {
  FieldCursors field_cursors;
  for (auto& field_assign : queries) {
    const std::string& field = field_assign.name();
    FieldMeta* meta = index_->GetMeta(field);
    ExprParser* parser = meta->parser.get();
    if (meta == nullptr || parser == nullptr) {
      continue;
    }
    EntriesContainer* container = index_->GetContainer(field);
    if (meta == nullptr) {
      continue;
    }
    TokenPtr token = parser->ParseQueryAssign(field_assign.Values());
    if (!token || token->BadToken()) {
      LOG(ERROR) << "query assign parse fail:" << token->FailReason();
      continue;
    }
    auto entries_list = container->RetrieveEntries(meta, token.get());
    FieldCursorPtr field_cursor(new FieldCursor());
    for (auto& entries : entries_list) {
      Attr attr = std::make_pair(field, 0);
      field_cursor->AddEntries(attr, entries);
    }
    if (field_cursor->Size() > 0) {
      field_cursor->Initialize();
      field_cursors.push_back(std::move(field_cursor));
    }
  }
  auto wc_cursor = index_->GenWildcardCursor();
  if (wc_cursor) {
    wc_cursor->Initialize();
    field_cursors.push_back(std::move(wc_cursor));
  }
  return field_cursors;
}

IndexScanner::Result IndexScanner::Retrieve(const QueryAssigns& queries,
                                            const Option* opt) const {
  if (nullptr == opt) {
    opt = &default_option;
  }
  IndexScanner::Result result;

  FieldCursors field_cursors = init_cursors(queries, opt);

LOOP:
  do {
    std::sort(field_cursors.begin(),
              field_cursors.end(),
              [](FieldCursorPtr& l, FieldCursorPtr& r) -> bool {
                return l->GetCurEntryID() < r->GetCurEntryID();
              });
    // remove those entries that have already reached end;
		// the end-up cursor will in the end of slice after sorting
    int idx = field_cursors.size() - 1;
    while (!field_cursors.empty() && field_cursors.back()->ReachEnd()) {
      field_cursors.pop_back();
    }

    if (field_cursors.empty()) {
      return result;
    }

    EntryId eid = field_cursors[0]->GetCurEntryID();
    uint64_t conj_id = EntryUtil::GetConjunctionId(eid);

    size_t k = ConjUtil::GetConjunctionSize(conj_id);
    if (0 == k) {
      k = 1;
    }

    if (k > field_cursors.size()) {
      break;
    }

    EntryId end_eid = field_cursors[k-1]->GetCurEntryID();
    uint64_t end_conj_id = EntryUtil::GetConjunctionId(eid);
    EntryId next_eid = EntryUtil::GenEntryID(EntryUtil::GetConjunctionId(end_eid), true);
    if (end_conj_id == conj_id) {
      next_eid = end_eid + 1;
      if (EntryUtil::IsInclude(eid)) {
        result.result.push_back(EntryUtil::GetDocID(eid));
      } else {
        for (size_t j = k; j < field_cursors.size(); j++) {
          if (conj_id != field_cursors[j]->GetCurConjID()) {
            break;
          }
          field_cursors[j]->Skip(next_eid);
        }
      }
    }

    for (size_t i = 0; i < k; i++) {
      field_cursors[i]->SkipTo(next_eid);
    }
  } while(1);

  return result;
}

}  // namespace component
