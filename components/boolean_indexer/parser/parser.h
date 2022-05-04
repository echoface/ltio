#ifndef _LT_COMPONENT_BOOLEAN_INDEXER_PARSER_H_
#define _LT_COMPONENT_BOOLEAN_INDEXER_PARSER_H_

#include <memory>

#include "components/boolean_indexer/document.h"
#include "components/boolean_indexer/id_generator.h"

namespace component {

/* Token as container input data
 * IndexBuilder -parser-> Token --> EntriesContainer --> PostingList
 * now just two type string/int supported, maybe upgrade to std::any/varaint
 * */
class Token {
public:
  virtual ~Token(){};

  bool BadToken() const { return fail_; };

  void SetFail(const std::string& reason) {
    fail_ = true;
    reason_ = reason;
  }

  const std::string& FailReason() const { return reason_; }

  virtual std::vector<int64_t> Int64() const { return {}; };

  virtual std::vector<std::string> Strs() const { return {}; };

protected:
  bool fail_ = false;
  std::string reason_;
};

struct TokenI64 : public Token {
  std::vector<int64_t> Int64() const override { return values_; };
  std::vector<int64_t> values_;
};

class TokenStr : public Token {
  std::vector<std::string> Strs() const override { return values_; };
  std::vector<std::string> values_;
};
using TokenPtr = std::unique_ptr<Token>;

class ExprParser {
public:
  virtual ~ExprParser() {};

  virtual TokenPtr ParseIndexing(const AttrValues::ValueContainer& values) = 0;

  virtual TokenPtr ParseQueryAssign(const AttrValues::ValueContainer& values) const = 0;
};
using RefExprParser = std::shared_ptr<ExprParser>;


}  // namespace component
#endif
