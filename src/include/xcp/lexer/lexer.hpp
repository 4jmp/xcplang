#pragma once
#include "xcp/lexer/token.hpp"
#include <string>
#include <vector>
namespace xcp::lexer {
class Lexer {
public:
  explicit Lexer(std::string source);
  std::vector<Token> scan();

private:
  std::string source_;
  std::size_t start_ = 0, current_ = 0, line_ = 1, column_ = 1;
  void scan_token(std::vector<Token> &);
  char advance();
  bool match(char);
  void skip_space();
};
} // namespace xcp::lexer
