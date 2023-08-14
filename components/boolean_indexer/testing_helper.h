#ifndef _LT_COMPONENT_BE_INDEXER_TESTING_HELPER
#define _LT_COMPONENT_BE_INDEXER_TESTING_HELPER

#include <ostream>
#include "components/boolean_indexer/document.h"

namespace component {

AttrValues::ValueContainer GenRandValues(int n, int min, int max);

bool EvalBoolExpression(const BooleanExpr& expr, AttrValues ass);

struct UTBooleanExpr {
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

std::ostream& operator<<(std::ostream& os, const UTBooleanExpr& t);

}  // namespace component
#endif
