#include <ostream>

#include "testing_helper.h"

#include "base/utils/rand_util.h"
#include "components/boolean_indexer/document.h"

namespace component {

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

AttrValues::ValueContainer GenRandValues(int n, int min, int max) {
  AttrValues::ValueContainer expr_assigns;
  while (n-- >= 0) {
    int v = base::RandInt(min, max);
    expr_assigns.insert(std::to_string(v));
  };
  return expr_assigns;
}

std::ostream& operator<<(std::ostream& os, const UTBooleanExpr& t) {
  os << ">>>>>>>>>t:" << t.id << "\na:" << t.a << "\nb:" << t.b << "\nc:" << t.c
     << "\n";
  return os;
}

}  // namespace component
