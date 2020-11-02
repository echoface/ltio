#ifndef _LT_COMPONENT_BOOLEAN_INDEXER_DOC_H_
#define _LT_COMPONENT_BOOLEAN_INDEXER_DOC_H_

#include <cstdint>
#include <map>
#include <memory>
#include <vector>
#include <iostream>

namespace component {


class BooleanExpr {
  public:
    typedef std::string ExprValue;
    typedef std::vector<ExprValue> ValueContainer;
    typedef std::initializer_list<ExprValue> ValueInitList;

    BooleanExpr(std::string name, const ValueInitList& values)
      : BooleanExpr(name, false, values) {
    }
    BooleanExpr(std::string name, bool exclude, const ValueInitList& values);
    ~BooleanExpr() {};

    const ValueContainer& Values() const {
      return values_;
    };
    void SetValue(const ValueInitList& values) {
      values_.clear(), AppendValue(values);
    }

    void AppendValue(ExprValue value) {
      values_.insert(values_.end(), value);
    }

    void AppendValue(const ValueInitList& values) {
      values_.insert(values_.end(), values.begin(), values.end());
    }

    std::string name() const {return name_;}
    bool exclude() const {return exclude_;}
  private:
    friend std::ostream& operator<< (std::ostream& os, const BooleanExpr& expr);

  private:
    bool exclude_; //inc, exc
    std::string name_;
    ValueContainer values_;
};

std::ostream& operator<< (std::ostream& os, const BooleanExpr& expr);

class Conjunction {
  public:
    Conjunction(){};
    ~Conjunction() {};

    void SetConjunctionId(uint64_t id);
    void AddExpression(const BooleanExpr& expr);

    size_t size() const {return size_;}
    uint64_t conjunction_id() const {return con_id_;}
  private:
    friend std::ostream& operator<< (std::ostream& os, const Conjunction& conj);
    size_t size_ = 0;
    uint64_t con_id_ = 0;
    std::map<std::string, BooleanExpr> assigns_;
};

typedef std::unique_ptr<Conjunction> ConjunctionPtr;
std::ostream& operator<< (std::ostream& os, const Conjunction& expr);

class Document {
  public:
    Document(int32_t doc_id);
    Document(Document&& doc)
      : doc_id_(doc.doc_id_),
        conjunctions_(doc.conjunctions_) {
    }
    ~Document();
    Document& operator=(Document&& doc) {
      if (this == &doc) {
        return *this;
      }
      doc_id_ = doc.doc_id_;
      conjunctions_ = std::move(doc.conjunctions_);
      return *this;
    }

    int32_t doc_id() const {return doc_id_;}
    const std::vector<Conjunction*>& conjunctions() const {
      return conjunctions_;
    };

    size_t ConjunctionCount() const {return conjunctions_.size();}

    void AddConjunction(Conjunction* con);
  private:
    std::int32_t doc_id_;
    std::vector<Conjunction*> conjunctions_;
};

typedef std::unique_ptr<Document> DocumentPtr;
std::ostream& operator<< (std::ostream& os, const Document& doc);

}
#endif
