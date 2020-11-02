#include <stdlib.h>
#include <unistd.h>
#include <atomic>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include "components/boolean_indexer/document.h"
#include "components/boolean_indexer/id_generator.h"

#include <thirdparty/catch/catch.hpp>

TEST_CASE("conjunction_gen_id", "[document and conjunction id]") {

  uint64_t conj_id = component::GenConjunctionId(120, 0, 5);
  component::ConjunctionIdDesc detail(conj_id);
  REQUIRE(detail.doc_id == 120);
  REQUIRE(detail.conj_size == 5);
  REQUIRE(detail.idx_in_doc == 0);
}

TEST_CASE("conjunction_dump", "[dump document and conjunction id]") {

  component::BooleanExpr expr1("gender", true, {"man"});
  std::cout << expr1 << std::endl;

  component::BooleanExpr expr2("segid", {"man", "less", "house"});
  std::cout << expr2 << std::endl;

  component::Conjunction conj;
  conj.AddExpression(component::BooleanExpr("gender", {"man"}));
  conj.AddExpression(component::BooleanExpr("age", {"15", "20", "25"}));
  conj.SetConjunctionId(component::GenConjunctionId(120, 0, conj.size()));
  std::cout << conj << std::endl;

  component::Conjunction conj2;
  conj2.AddExpression(component::BooleanExpr("iplist", {"15", "20", "25"}));
  conj2.AddExpression(component::BooleanExpr("seglist", {"110", "157", "man_25_30"}));
  conj2.AddExpression(component::BooleanExpr("exclude", true, {"item", "target", "object"}));
  conj2.SetConjunctionId(component::GenConjunctionId(120, 1, conj.size()));
  std::cout << conj2 << std::endl;
}

