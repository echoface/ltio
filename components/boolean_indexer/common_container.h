#ifndef _LT_COMPONENT_BE_ENTRIES_COMMON_CONTAINER_H_
#define _LT_COMPONENT_BE_ENTRIES_COMMON_CONTAINER_H_

#include "posting_list.h"

namespace component {

class CommonContainer : public EntriesContainer {
public:
  CommonContainer(){};

  bool IndexingToken(const FieldMeta* meta,
                     const EntryId eid,
                     const Token* token) override;

  EntriesList RetrieveEntries(const FieldMeta* meta,
                              const Token* token) const override;

  bool CompileEntries() override;

  void DumpEntries(std::ostringstream& oss) const override;

  int64_t Size() const { return values_.size(); }

  int64_t MaxEntriesLength() const { return max_len_; }

  int64_t AvgEntriesLength() const { return sum_len_ / Size(); }

private:
  int max_len_ = 0;
  int sum_len_ = 0;
  PostingList values_;
};

}  // namespace component
#endif
