#pragma once
#include "xcp/lexer/token.hpp"
#include "xcp/parser/ast.hpp"
#include <stdexcept>
#include <vector>
namespace xcp::parser {
class Parser {
public:
  explicit Parser(std::vector<lexer::Token> tokens);
  ast::Program parse();

private:
  std::vector<lexer::Token> tokens_;
  std::size_t current_ = 0;
  const lexer::Token &peek() const;
  bool match(const std::string &);
  void expect(const std::string &);
  ast::StatementPtr statement();
  ast::ExprPtr expression();
  ast::ExprPtr term();
  ast::ExprPtr primary();
};
} // namespace xcp::parser
