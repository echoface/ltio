#include "common_container.h"

#include <iostream>
#include <sstream>

#include "fmt/format.h"
#include <thirdparty/catch/catch.hpp>

using namespace component;

TEST_CASE("commoncontainer", "[test defautl container]") {
  CommonContainer con;

  uint64_t conj_id = ConjUtil::GenConjID(-100, 1, 1);
  fmt::print("conj doc:{}, idx:{} size:{} \n",
             ConjUtil::GetDocumentID(conj_id),
             ConjUtil::GetIndexInDoc(conj_id),
             ConjUtil::GetConjunctionSize(conj_id));

  EntryId eid = EntryUtil::GenEntryID(conj_id, false);
  fmt::print("entry doc:{}, size:{} \n",
             EntryUtil::GetDocID(eid),
             EntryUtil::GetConjSize(eid));


  TokenI64 token;
  token.values_ = {1, 2, 2, 3, 4};

  FieldMeta meta;
  meta.name = "age";

  con.IndexingToken(&meta, eid, &token);

  REQUIRE(con.CompileEntries() == true);

  REQUIRE(con.Size() == 4);
  REQUIRE(con.MaxEntriesLength() == 2);

  token.values_ = {1, 2, 10086};
  auto plList = con.RetrieveEntries(&meta, &token);
  REQUIRE(plList.size() == 2);
  std::ostringstream oss;
  con.DumpEntries(oss);
  fmt::print(oss.str());
}
