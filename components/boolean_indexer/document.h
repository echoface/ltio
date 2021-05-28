#ifndef _LT_COMPONENT_BOOLEAN_INDEXER_DOC_H_
#define _LT_COMPONENT_BOOLEAN_INDEXER_DOC_H_

#include <cstdint>
#include <map>
#include <memory>
#include <iostream>
#include <set>
#include <vector>

namespace component {

class Assigns {
  public:
    typedef std::string ExprValue;
    typedef std::set<ExprValue> ValueContainer;
    typedef std::initializer_list<ExprValue> ValueInitList;

    Assigns(const std::string& name, const ValueInitList& values)
      : name_(name),
      values_(values) {
    }

    Assigns(const std::string& name, const ValueContainer& values)
      : name_(name),
      values_(values) {
    }

    void SetValue(const ValueInitList& values) {
      values_.clear(), AppendValue(values);
    }
    void AppendValue(ExprValue value) {
      values_.insert(value);
    }
    void AppendValue(const ValueInitList& values) {
      values_.insert(values);
    }

    const ValueContainer& Values() const {
      return values_;
    };

    std::string name() const {return name_;}
  protected:
    std::string name_;
    ValueContainer values_;
};

class BooleanExpr : public Assigns {
  public:
    typedef std::initializer_list<BooleanExpr> BEInitializer;

    BooleanExpr(const std::string& name,
                const ValueInitList& values,
                bool exclude = false);
    BooleanExpr(const std::string& name,
                const ValueContainer& values,
                bool exclude = false);

    ~BooleanExpr() {};

    bool exclude() const {return exclude_;}
    bool include() const {return !exclude_;}
  private:
    friend std::ostream& operator<< (std::ostream& os, const BooleanExpr& expr);

  private:
    bool exclude_; //inc, exc
};

std::ostream& operator<< (std::ostream& os, const BooleanExpr& expr);

class Conjunction {
  public:
    Conjunction(){};
    Conjunction(const BooleanExpr::BEInitializer& exprs){
      AddExpression(exprs);
    };
    ~Conjunction() {};

    void SetConjunctionId(uint64_t id);

    void AddExpression(const BooleanExpr& expr);

    void AddExpression(const BooleanExpr::BEInitializer& exprs);

    Conjunction* Include(const std::string& field,
                         const Assigns::ValueInitList& values);

    Conjunction* Exclude(const std::string& field,
                         const Assigns::ValueInitList& values);

    std::map<std::string, BooleanExpr> ExpressionAssigns() const {
        return assigns_;
    };

    size_t size() const {return size_;}
    uint64_t id() const {return con_id_;}

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
        conjunctions_(std::move(doc.conjunctions_)) {
    }

    Document(int32_t doc_id, Conjunction* conj)
      : doc_id_(doc_id) {
        AddConjunction(conj);
    }

    ~Document();

    Document& operator=(Document&& doc) {
      if (this == &doc) {
        return *this;
      }
      doc_id_ = doc.doc_id_;
      conjunctions_ = std::move(doc.conjunctions_);

      doc.doc_id_ = 0;
      doc.conjunctions_.clear();
      return *this;
    }

    int32_t doc_id() const {return doc_id_;}

    const std::vector<Conjunction*>& conjunctions() const {
      return conjunctions_;
    };

    void AddConjunction(Conjunction* con);
  private:
    std::int32_t doc_id_;

    std::vector<Conjunction*> conjunctions_;
};

typedef std::unique_ptr<Document> DocumentPtr;
std::ostream& operator<< (std::ostream& os, const Document& doc);

}
#endif
