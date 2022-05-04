#ifndef _LT_COMPONENT_BOOLEAN_INDEXER_NUMBER_PARSER_H_
#define _LT_COMPONENT_BOOLEAN_INDEXER_NUMBER_PARSER_H_

#include "parser.h"

namespace component {

class NumberParser : public ExprParser {
public:
  ~NumberParser() {};

  TokenPtr ParseIndexing(const AttrValues::ValueContainer& values) override ;

  TokenPtr ParseQueryAssign(const AttrValues::ValueContainer& values) const override;
private:
};

// hasher parser encode str values into int64 id list token(aka: TokenI64)
class HasherParser : public ExprParser {
public:
  ~HasherParser() {};

  TokenPtr ParseIndexing(const AttrValues::ValueContainer& values) override ;

  TokenPtr ParseQueryAssign(const AttrValues::ValueContainer& values) const override;
private:
  std::hash<std::string> hasher_;
};

} // end namespace

#endif
