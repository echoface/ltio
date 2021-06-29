#include "document.h"
#include "id_generator.h"

#include <cassert>

namespace component {

BooleanExpr::BooleanExpr(const std::string& name,
                         const ValueInitList& values,
                         bool exclude)
  : Assigns(name, values), exclude_(exclude) {}

BooleanExpr::BooleanExpr(const std::string& name,
                         const ValueContainer& values,
                         bool exclude)
  : Assigns(name, values), exclude_(exclude) {}

std::ostream& operator<<(std::ostream& os, const BooleanExpr& expr) {
  os << "{" << expr.name() << (expr.exclude() ? " exc [" : " inc [");

  const Assigns::ValueContainer& values = expr.Values();
  for (auto iter = values.begin(); iter != values.end(); iter++) {
    os << *iter << ", ";
  }
  os << "]}";
  return os;
}

void Conjunction::SetConjunctionId(uint64_t id) {
  con_id_ = id;
}

void Conjunction::AddExpression(const BooleanExpr& expr) {
  auto result = assigns_.insert(std::make_pair(expr.name(), expr));
  // not allow dumplicated assign
  assert(result.second);
  size_ += (expr.exclude() ? 0 : 1);
}

void Conjunction::AddExpression(const BooleanExpr::BEInitializer& exprs) {
  for (const auto& expr : exprs) {
    AddExpression(expr);
  }
}

Conjunction* Conjunction::Include(const std::string& field,
                                  const Assigns::ValueInitList& values) {
  return this;
}

Conjunction* Conjunction::Exclude(const std::string& field,
                                  const Assigns::ValueInitList& values) {
  return this;
}

std::ostream& operator<<(std::ostream& os, const Conjunction& conj) {
  os << ">>>>>"
     << " id:" << conj.id() << " (size:" << conj.size()
     << ", doc:" << ConjUtil::GetDocumentID(conj.id())
     << ", idx:" << ConjUtil::GetIndexInDoc(conj.id()) << ") >>>>> \n";
  for (const auto& name_expr : conj.assigns_) {
    os << name_expr.second << "\n";
  }
  os << "\n";
  return os;
}

Document::Document(int32_t doc_id) : doc_id_(doc_id) {}

Document::~Document() {
  for (Conjunction*& conj : conjunctions_) {
    delete conj;
    conj = nullptr;
  }
}

void Document::AddConjunction(Conjunction* conj) {
  uint64_t conj_id =
      ConjUtil::GenConjID(doc_id(), conjunctions_.size(), conj->size());

  conj->SetConjunctionId(conj_id);

  conjunctions_.push_back(conj);
}

std::ostream& operator<<(std::ostream& os, const Document& doc) {
  os << ">>>>> doc:" << doc.doc_id() << ">>>>>>\n";
  for (const auto& conj : doc.conjunctions()) {
    os << *conj;
  }
  os << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
  return os;
}

}  // namespace component
