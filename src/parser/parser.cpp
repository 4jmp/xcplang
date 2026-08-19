#include "xcp/parser/parser.hpp"
namespace xcp::parser {
Parser::Parser(std::vector<lexer::Token> t) : tokens_(std::move(t)) {}
const lexer::Token &Parser::peek() const { return tokens_[current_]; }
bool Parser::match(const std::string &s) {
  if (peek().lexeme == s) {
    ++current_;
    return true;
  }
  return false;
}
void Parser::expect(const std::string &s) {
  if (!match(s))
    throw std::runtime_error("expected " + s);
}
ast::ExprPtr Parser::primary() {
  auto t = peek();
  if (match("(")) {
    auto x = expression();
    expect(")");
    return x;
  }
  if (t.kind == lexer::Kind::number || t.kind == lexer::Kind::string) {
    ++current_;
    return std::make_unique<ast::Literal>(t.lexeme);
  }
  if (t.kind == lexer::Kind::identifier) {
    ++current_;
    return std::make_unique<ast::Name>(t.lexeme);
  }
  throw std::runtime_error("expected expression");
}
ast::ExprPtr Parser::term() {
  auto left = primary();
  while (peek().lexeme == "*" || peek().lexeme == "/") {
    auto op = peek().lexeme;
    ++current_;
    left = std::make_unique<ast::Binary>(std::move(left), op, primary());
  }
  return left;
}
ast::ExprPtr Parser::expression() {
  auto left = term();
  while (peek().lexeme == "+" || peek().lexeme == "-") {
    auto op = peek().lexeme;
    ++current_;
    left = std::make_unique<ast::Binary>(std::move(left), op, term());
  }
  return left;
}
ast::StatementPtr Parser::statement() {
  if (match("let")) {
    auto n = peek().lexeme;
    ++current_;
    expect("=");
    return std::make_unique<ast::Let>(n, expression());
  }
  expression();
  return nullptr;
}
ast::Program Parser::parse() {
  ast::Program p;
  while (peek().kind != lexer::Kind::end) {
    if (auto s = statement())
      p.statements.push_back(std::move(s));
  }
  return p;
}
} // namespace xcp::parser
