#include <stdlib.h>
#include <unistd.h>
#include <atomic>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "base/time/time_utils.h"
#include "base/utils/rand_util.h"
#include "components/boolean_indexer/builder/be_indexer_builder.h"
#include "components/boolean_indexer/document.h"
#include "components/boolean_indexer/id_generator.h"
#include "components/boolean_indexer/index_scanner.h"
#include "components/boolean_indexer/testing_helper.h"

#include <catch2/catch_test_macros.hpp>
#include "gperftools/profiler.h"

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
  conj2.AddExpression(
      BooleanExpr("exclude", {"item", "target", "object"}, true));
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

  Document doc2(2);
  doc2.AddConjunction(new Conjunction({
      {"loc", {"bj"}},
      {"tags", {"seg1", "seg2"}},
  }));
  builder.AddDocument(std::move(doc2));

  Document doc3(3);
  doc3.AddConjunction(new Conjunction({
      {"loc", {"bj"}, true},
  }));
  builder.AddDocument(std::move(doc3));

  auto indexer = builder.BuildIndexer();
  std::ostringstream oss;
  indexer->DumpIndex(oss);
  std::cout << "pl detail:\n" << oss.str() << std::endl;

  std::vector<QueryAssigns> queries = {
      {{"loc", {"bj"}}, {"tags", {"15sui", "seg1"}}},  //=> 10
      {{"loc", {"wh"}}, {"tags", {"15sui", "seg1"}}},  //=> 100
  };

  std::cout << "start query" << std::endl;
  for (auto& query : queries) {
    auto scanner = component::IndexScanner(indexer.get());
    auto r = scanner.Retrieve(query);

    std::cout << "query retrieve docs:";
    for (auto& doc : r.result) {
      std::cout << doc << ",";
    }
    std::cout << std::endl;
  }
}

TEST_CASE("be_posting_list", "[be posting list sort id]") {
  Entries entrylist = {0, 1, 1, 4, 8, 20};

  Attr attr("age", 0);
  EntriesCursor cursor(attr, entrylist);

  REQUIRE(cursor.ReachEnd() == false);
  REQUIRE(cursor.GetCurEntryID() == 0);
  REQUIRE(cursor.Skip(1) == 4);
  REQUIRE(cursor.GetCurEntryID() == 4);
  REQUIRE(cursor.Skip(5) == 8);
  REQUIRE(cursor.GetCurEntryID() == 8);
  REQUIRE(cursor.SkipTo(20) == 20);
  REQUIRE(cursor.GetCurEntryID() == 20);
  REQUIRE(cursor.Skip(20) == NULLENTRY);
  REQUIRE(cursor.GetCurEntryID() == NULLENTRY);
  REQUIRE(cursor.ReachEnd());
}

TEST_CASE("entries_cursor", "[skip posting list sort id]") {
  Entries ids = {1, 18, 24, 57, 70};
  Attr attr("test", 0);

  EntriesCursor curosr(attr, ids);

  EntriesCursor c2 = curosr;
}

TEST_CASE("skipto_test", "[skip posting list sort id]") {
  //[<1,1>,<18,1>,<24,1>,<57,1>,<70,1>,]
  Entries ids = {1, 18, 24, 57, 70};
  Attr attr("test", 0);
  EntriesCursor iter(attr, ids);

  int res = iter.SkipTo(2);
  REQUIRE(res == 18);
}

TEST_CASE("index_result_check", "[be posting list correction check]") {
  std::map<int, UTBooleanExpr*> targets;
  component::BeIndexerBuilder builder;

  for (int i = 1; i < 10000; i++) {
    auto a = GenRandValues(10, 0, 100);
    UTBooleanExpr* t = new UTBooleanExpr({
        .id = i,
        .a = {"a", GenRandValues(5, 50, 100), base::RandInt(0, 100) > 80},
        .b = {"b", GenRandValues(10, 0, 100), base::RandInt(0, 100) > 80},
        .c = {"c", GenRandValues(8, 10, 70), base::RandInt(0, 100) > 80},
    });

    targets[i] = t;
    Document doc(i);
    doc.AddConjunction(new Conjunction({t->a, t->b, t->c}));
    builder.AddDocument(std::move(doc));
  }
  auto index = builder.BuildIndexer();

  auto none_index_match = [&](QueryAssigns& assigns) -> std::set<int32_t> {
    std::set<int32_t> result;
    for (const auto& t : targets) {
      int id = t.first;
      if (t.second->match(assigns)) {
        result.insert(id);
      }
    }
    return result;
  };
  std::cout << "finish index build" << std::endl;

  std::vector<QueryAssigns> queries;
  for (int i; i < 10000; i++) {
    QueryAssigns assigns = {
        {"a", GenRandValues(1, 50, 100)},
        {"b", GenRandValues(2, 0, 100)},
        {"c", GenRandValues(3, 10, 70)},
    };
    queries.push_back(assigns);
  }
  for (int i = 0; i < queries.size(); i++) {
    auto scanner = IndexScanner(index.get());

    auto result = scanner.Retrieve(queries[i]);
    auto force_match_result = none_index_match(queries[i]);

    std::set<int32_t> index_result(result.result.begin(), result.result.end());
    std::set<int32_t> diff;
    std::set_difference(index_result.begin(), index_result.end(),
                        force_match_result.begin(), force_match_result.end(),
                        std::inserter(diff, diff.end()));
    std::set<int32_t> diff2;
    std::set_difference(force_match_result.begin(), force_match_result.end(),
                        index_result.begin(), index_result.end(),
                        std::inserter(diff2, diff2.end()));

    if (diff.size() || diff2.size()) {
      std::ostringstream oss;
      index->DumpIndex(oss);

      IndexScanner::Option option;
      option.dump_detail = true;
      scanner.Retrieve(queries[i], &option);

      oss << "more:[\n";
      for (auto x : diff) {
        oss << *targets[x] << ",\n";
      }
      oss << "] less:[\n";
      for (auto x : diff2) {
        oss << *targets[x] << ",\n";
      }
      oss << "]\nquery:\n";
      for (auto& q : queries[i]) {
        oss << "{name:" << q.name() << " values:";
        for (auto& v : q.Values()) {
          oss << v << ",";
        }
        oss << "}\n";
      }
      std::cout << oss.str() << std::endl;
      REQUIRE(false);
    }
  }
}

TEST_CASE("index_build", "[build posting list correction check]") {
  /*
   * >>>>>>>>>t:8
a:{a inc [55, 56, 65, 76, 92, 93, ]}
b:{b inc [15, 18, 46, 47, 68, 75, 84, 93, 97, ]}
c:{c exc [14, 16, 30, 31, 39, 51, 58, 66, ]}
query:
{name:a values:67,70,}
{name:b values:30,46,82,}
{name:c values:42,55,63,67,}
   * */
  std::ostringstream oss;
  UTBooleanExpr* t = new UTBooleanExpr({
      .id = 8,
      .a = {"a", {"55", "56", "65", "76", "92", "93"}, false},
      .b = {"b", {"15", "18", "46", "47", "68", "75", "84", "93", "97"}, false},
      .c = {"c", {"14", "16", "30", "31", "39", "51", "58", "66"}, true},
  });

  BeIndexerBuilder builder;
  Document doc(8);
  doc.AddConjunction(new Conjunction({t->a, t->b, t->c}));
  builder.AddDocument(std::move(doc));

  auto index = builder.BuildIndexer();
  index->DumpIndex(oss);

  IndexScanner::Option option;
  option.dump_detail = true;
  IndexScanner scanner(index.get());
  auto r = scanner.Retrieve(
      {
          {"a", {"67", "70"}},
          {"b", {"30", "46", "82"}},
          {"c", {"42", "55", "63", "67"}},
      },
      &option);
  oss << r.to_string();
  std::cout << oss.str();
}
