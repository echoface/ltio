#include <stdlib.h>
#include <unistd.h>
#include <atomic>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include "components/boolean_indexer/document.h"
#include "components/boolean_indexer/id_generator.h"
#include "components/boolean_indexer/builder/be_indexer_builder.h"

#include <thirdparty/catch/catch.hpp>

using namespace component;

TEST_CASE("conjunction_gen_id", "[document and conjunction id]") {

  uint64_t conj_id = ConjUtil::GenConjID(120, 0, 5);
  REQUIRE(120 == ConjUtil::GetDocumentID(conj_id));
  REQUIRE(0 == ConjUtil::GetIndexInDoc(conj_id));
  REQUIRE(5 == ConjUtil::GetConjunctionSize(conj_id));
}

TEST_CASE("conjunction_dump", "[dump document and conjunction id]") {

  component::BooleanExpr expr1("gender", {"man"}, true);
  std::cout << expr1 << std::endl;

  component::BooleanExpr expr2("segid", {"man", "less", "house"});
  std::cout << expr2 << std::endl;

  component::Conjunction conj;
  conj.AddExpression(BooleanExpr("gender", {"man"}));
  conj.AddExpression(BooleanExpr("age", {"15", "20", "25"}));
  conj.SetConjunctionId(ConjUtil::GenConjID(120, 0, conj.size()));
  std::cout << conj << std::endl;

  component::Conjunction conj2;
  conj2.AddExpression(BooleanExpr("iplist", {"15", "20", "25"}));
  conj2.AddExpression(BooleanExpr("seglist", {"110", "157", "man_25_30"}));
  conj2.AddExpression(BooleanExpr("exclude", {"item", "target", "object"}, true));
  conj2.SetConjunctionId(ConjUtil::GenConjID(120, 1, conj.size()));
  std::cout << conj2 << std::endl;
}


TEST_CASE("be_indexes_dump", "[dump indexes and conjunction id]") {

  BeIndexerBuilder builder;

  Document doc(1);
  doc.AddConjunction(new Conjunction({
        {"age", {"15"}},
        {"loc", {"sh"}},
        {"sex", {"man"}},
        {"tags", {"seg2"}},
      }));
  doc.AddConjunction(new Conjunction({
        {"age", {"15"}},
        {"loc", {"sh"}},
        {"sex", {"female"}, true},
        {"tags", {"seg1", "seg2"}},
      }));
  builder.AddDocument(std::move(doc));

  Document doc2(10);
  doc2.AddConjunction(new Conjunction(
      {
        {"loc", {"bj"}},
        {"tags", {"seg1", "seg2"}},
      }));
  builder.AddDocument(std::move(doc2));

  Document doc3(100);
  doc3.AddConjunction(new Conjunction(
      {
        {"loc", {"bj"}, true},
      }));
  builder.AddDocument(std::move(doc3));

  auto indexer = builder.BuildIndexer();

  std::vector<QueryAssigns> queries = {
    {
      {"loc", {"bj"}},
      {"tags", {"15sui", "seg1"}}
    },
    {
      {"loc", {"wh"}},
      {"tags", {"15sui", "seg1"}}
    },
  };

  std::set<int32_t> result;
  for (auto& query : queries) {
    result.clear();
    std::cout << "retrieve docs:\n";
    indexer->Query(query, &result);
    for (auto& doc : result) {
      std::cout << "got doc:" << doc << "\n";
    }
  }
}

TEST_CASE("be_posting_list", "[be posting list sort id]") {
  Entries entrylist= {0, 1, 1, 4, 8, 20};

  Attr attr("age", 0);
  EntriesIterator plist(attr, 0, &entrylist);

  REQUIRE(plist.ReachEnd() == false);
  REQUIRE(plist.GetCurEntryID() == 0);
  REQUIRE(plist.Skip(1) == 4);
  REQUIRE(plist.GetCurEntryID() == 4);
  REQUIRE(plist.Skip(5) == 8);
  REQUIRE(plist.GetCurEntryID() == 8);
  REQUIRE(plist.SkipTo(20) == 20);
  REQUIRE(plist.GetCurEntryID() == 20);
  REQUIRE(plist.Skip(20) == NULLENTRY);
  REQUIRE(plist.GetCurEntryID() == NULLENTRY);
  REQUIRE(plist.ReachEnd());

  PostingListGroup group;
}

