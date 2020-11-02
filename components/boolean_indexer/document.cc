#include "document.h"
#include "id_generator.h"

#include <cassert>

namespace component {

BooleanExpr::BooleanExpr(std::string name, bool exclude, const ValueInitList& values)
  : name_(name),
    exclude_(exclude) {
  SetValue(values);
}

std::ostream& operator<< (std::ostream& os, const BooleanExpr& expr) {
  os << "{" << expr.name() << (expr.exclude() ? " exc [" : " inc [");

  const BooleanExpr::ValueContainer& values = expr.Values();
  for (size_t i = 0; i < values.size(); i++) {
    os << values.at(i);
    if (i < values.size() - 1) os << ";";
  }
  os << "]}";
  return os;
}

void Conjunction::SetConjunctionId(uint64_t id) {
  con_id_ = id;
}

void Conjunction::AddExpression(const BooleanExpr& expr) {
  auto result = assigns_.insert(std::make_pair(expr.name(), expr));

  //not allow dumplicated assign
  assert(result.second);

  size_ += (expr.exclude() ? 0 : 1);
}

std::ostream& operator<< (std::ostream& os, const Conjunction& conj) {
  component::ConjunctionIdDesc desc(conj.con_id_);

  os << ">>>>>"
    << " id:" << conj.con_id_
    << " (size:" << conj.size()
    << ", doc:" << desc.doc_id
    << ", idx:" << uint32_t(desc.idx_in_doc)
    << ") >>>>> \n";
  for (const auto& name_expr : conj.assigns_) {
    os << name_expr.second << "\n";
  }
  os << "\n";
  return os;
}

Document::~Document() {
  for (Conjunction* &conj : conjunctions_) {
    delete conj;
    conj = nullptr;
  }
}
void Document::AddConjunction(Conjunction* con) {
  if (con->size() == 0) {
    con->AddExpression({"__wildcard__", {"wildcard"}});
  }
  conjunctions_.push_back(con);
}

std::ostream& operator<< (std::ostream& os, const Document& doc) {
  os << ">>>>> doc:" << doc.doc_id() << ">>>>>>\n";
  for (const auto& conj : doc.conjunctions()) {
    os << *conj;
  }
  os << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
  return os;
}

}
