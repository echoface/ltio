#ifndef _LT_COMPONENT_BE_MOCK_H_
#define _LT_COMPONENT_BE_MOCK_H_

#include "base/utils//rand_util.h"
#include "components/boolean_indexer/document.h"
#include "components/boolean_indexer/posting_list.h"

namespace component {

AttrValues::ValueContainer rand_assigns(int n, int min, int max) {
  AttrValues::ValueContainer expr_assigns;
  while (n-- >= 0) {
    int v = base::RandInt(min, max);
    expr_assigns.insert(std::to_string(v));
  };
  return expr_assigns;
}

bool EvalBoolExpression(const BooleanExpr& expr, AttrValues ass) {
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
  os << ">>>>>>>>>t:" << t.id << "\na:" << t.a << "\nb:" << t.b << "\nc:" << t.c
     << "\n";
  return os;
}

};  // namespace component
#endif
