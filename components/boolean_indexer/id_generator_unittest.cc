#include <iostream>

#include "id_generator.h"

#include <thirdparty/catch/catch.hpp>

using namespace component;

TEST_CASE("gen_conj_id", "[document and conjunction id]") {
  uint64_t conj_id = ConjUtil::GenConjID(120, 0, 5);
  std::cout << "conj id:" << EntryUtil::ToString(conj_id) << std::endl;
  REQUIRE(120 == ConjUtil::GetDocumentID(conj_id));
  REQUIRE(0 == ConjUtil::GetIndexInDoc(conj_id));
  REQUIRE(5 == ConjUtil::GetConjunctionSize(conj_id));

  conj_id = ConjUtil::GenConjID(-120, 255, 5);
  std::cout << "conj id:" << EntryUtil::ToString(conj_id) << std::endl;
  REQUIRE(-120 == ConjUtil::GetDocumentID(conj_id));
  REQUIRE(255 == ConjUtil::GetIndexInDoc(conj_id));
  REQUIRE(5 == ConjUtil::GetConjunctionSize(conj_id));
}
TEST_CASE("gen_entry_id", "[document and conjunction id]") {
  uint64_t eid = EntryUtil::GenEntryID(120, true);
  REQUIRE_FALSE(EntryUtil::IsInclude(eid));
  REQUIRE(120 == EntryUtil::GetConjunctionId(eid));
  std::cout << "eid:" << EntryUtil::ToString(eid) << std::endl;

  eid = EntryUtil::GenEntryID(0xFFFFFF, false);
  REQUIRE_FALSE(EntryUtil::IsExclude(eid));
  REQUIRE(0xFFFFFF == EntryUtil::GetConjunctionId(eid));

  std::cout << "eid:" << EntryUtil::ToString(eid) << std::endl;
}
