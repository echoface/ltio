#include <stdlib.h>
#include <unistd.h>
#include <atomic>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>

#include "base/utils//rand_util.h"
#include "base/time//time_utils.h"
#include "components/boolean_indexer/document.h"
#include "components/boolean_indexer/id_generator.h"
#include "components/boolean_indexer/builder/be_indexer_builder.h"
#include "components/boolean_indexer/index_scanner.h"

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

  Document doc2(2);
  doc2.AddConjunction(new Conjunction(
      {
        {"loc", {"bj"}},
        {"tags", {"seg1", "seg2"}},
      }));
  builder.AddDocument(std::move(doc2));

  Document doc3(3);
  doc3.AddConjunction(new Conjunction(
      {
        {"loc", {"bj"}, true},
      }));
  builder.AddDocument(std::move(doc3));

  auto indexer = builder.BuildIndexer();
  std::ostringstream oss;
  indexer->DumpIndex(oss);
  std::cout << "pl detail:\n" << oss.str() << std::endl;

  std::vector<QueryAssigns> queries = {
    {
      {"loc", {"bj"}},
      {"tags", {"15sui", "seg1"}}
    }, //=> 10
    {
      {"loc", {"wh"}},
      {"tags", {"15sui", "seg1"}}
    }, //=> 100
  };

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
  Entries entrylist= {0, 1, 1, 4, 8, 20};

  Attr attr("age", 0);
  EntriesCursor cursor(attr, &entrylist);

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

component::Assigns::ValueContainer rand_assigns(int n, int min, int max) {
  component::Assigns::ValueContainer expr_assigns;
  while(n-- >= 0) {
    int v = base::RandInt(min, max);
    expr_assigns.insert(std::to_string(v));
  };
  return expr_assigns;
}

bool EvalBoolExpression(const BooleanExpr& expr, Assigns ass) {
  if (expr.Values().empty()) {
    return true;
  }
  bool contain = false;
  for (const auto& v : expr.Values()) {
    for (const auto& qv : ass.Values()) {
      if (v == qv) {
        contain = true;
        goto outer;
      }
    }
  }
outer:
  if ((expr.exclude() && !contain) || (expr.include() && contain)) {
    return true;
  }
  return false;
}

struct T {
  int id;
  BooleanExpr a;
  BooleanExpr b;
  BooleanExpr c;
  bool match(QueryAssigns assigns) const {
    for (auto& assign : assigns) {
      if (assign.name() == "a" && !EvalBoolExpression(a, assign)) {
        return false;
      }
      if (assign.name() == "b" && !EvalBoolExpression(b, assign)) {
        return false;
      }
      if (assign.name() == "c" && !EvalBoolExpression(c, assign)) {
        return false;
      }
    }
    return true;
  }
};

std::ostream& operator<<(std::ostream& os, const T& t) {
  os << ">>>>>>>>>t:" << t.id
    << "\na:" << t.a
    << "\nb:" << t.b
    << "\nc:" << t.c << "\n";
  return os;
}


TEST_CASE("skipto_test", "[skip posting list sort id]") {
  //[<1,1>,<18,1>,<24,1>,<57,1>,<70,1>,]
  Entries ids = {1, 18, 24 ,57, 70}; 
  Attr attr("test", 0);
  EntriesCursor iter(attr, &ids);

  int res = iter.SkipTo(2);
  REQUIRE(res == 18);
}

TEST_CASE("index_rand_bench", "[be posting list sort id]") {
  std::map<int, T*> targets;
  component::BeIndexerBuilder builder;

  for (int i=1; i< 100000; i++) {
    auto a = rand_assigns(10, 0, 100);
    T* t = new T({
      .id = i,
      .a = {"a", rand_assigns(5, 50, 100), base::RandInt(0, 100) > 80},
      .b = {"b", rand_assigns(10, 0, 100), base::RandInt(0, 100) > 80},
      .c = {"c", rand_assigns(8, 10, 70), base::RandInt(0, 100) > 80},
    });

    targets[i] = t;
    Document doc(i);
    doc.AddConjunction(new Conjunction({t->a, t->b, t->c}));
    builder.AddDocument(std::move(doc));
  }
  auto index = builder.BuildIndexer();

  //std::ostringstream oss;
  //index->DumpIndex(oss);
  //std::cout << "posting list detail:" << oss.str() << std::endl;

  auto none_index_match = [&](QueryAssigns& assigns)-> std::set<int32_t> {
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

  uint64_t start = base::time_us();
  std::vector<QueryAssigns> queries;
  for (int i; i < 1000; i++) {
    QueryAssigns assigns = {
      {"a", rand_assigns(1, 50, 100)},
      {"b", rand_assigns(2, 0, 100)},
      {"c", rand_assigns(3, 10, 70)},
    };
    queries.emplace_back(assigns);
  }

  for (int i = 0; i < queries.size(); i++) {

    auto scanner = IndexScanner(index.get());

    auto result = scanner.Retrieve(queries[i]);
    //auto force_match_result = none_index_match(assigns);
    //std::ostringstream oss;
    //oss << "[";
    //for (auto x : force_match_result) {
    //  oss << x << ",";
    //}
    //oss << "]";
    //std::cout << "force:" << oss.str() << std::endl;
    //std::cout << "index:" << result.to_string() << std::endl;

    //std::set<int32_t> index_result(result.result.begin(), result.result.end());
    //if (index_result != force_match_result) {
    //  std::ostringstream oss;
    //  index->DumpIndex(oss);
    //  std::cout << "pl:" << oss.str();
    //}
    //REQUIRE(index_result == force_match_result);
  }
  std::cout << "spend:" << (base::time_us() - start) / 1000 << "(ms)" << std::endl;
}
