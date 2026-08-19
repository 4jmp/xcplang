#include "xcp/lexer/lexer.hpp"
#include <cctype>
#include <unordered_set>
namespace xcp::lexer {
const char *kind_name(Kind k) {
  switch (k) {
  case Kind::identifier:
    return "identifier";
  case Kind::number:
    return "number";
  case Kind::string:
    return "string";
  case Kind::keyword:
    return "keyword";
  case Kind::symbol:
    return "symbol";
  default:
    return "end";
  }
}
Lexer::Lexer(std::string s) : source_(std::move(s)) {}
char Lexer::advance() {
  char c = source_[current_++];
  if (c == '\n') {
    ++line_;
    column_ = 1;
  } else
    ++column_;
  return c;
}
bool Lexer::match(char c) {
  if (current_ >= source_.size() || source_[current_] != c)
    return false;
  ++current_;
  ++column_;
  return true;
}
void Lexer::skip_space() {
  while (current_ < source_.size()) {
    char c = source_[current_];
    if (std::isspace((unsigned char)c)) {
      advance();
      continue;
    }
    if (c == '#' || (c == '/' && current_ + 1 < source_.size() &&
                     source_[current_ + 1] == '/')) {
      while (current_ < source_.size() && source_[current_] != '\n')
        advance();
      continue;
    }
    if (c == '/' && current_ + 1 < source_.size() &&
        source_[current_ + 1] == '*') {
      advance();
      advance();
      while (current_ < source_.size()) {
        if (source_[current_] == '*' && current_ + 1 < source_.size() &&
            source_[current_ + 1] == '/') {
          advance();
          advance();
          break;
        }
        advance();
      }
      continue;
    }
    break;
  }
}
void Lexer::scan_token(std::vector<Token> &out) {
  skip_space();
  if (current_ >= source_.size())
    return;
  start_ = current_;
  auto line = line_, column = column_;
  char c = advance();
  if (std::isalpha((unsigned char)c) || c == '_') {
    while (current_ < source_.size() &&
           (std::isalnum((unsigned char)source_[current_]) ||
            source_[current_] == '_'))
      advance();
    auto s = source_.substr(start_, current_ - start_);
    static const std::unordered_set<std::string> keys = {
        "let",    "fn",     "if",   "else",  "while",
        "return", "import", "true", "false", "null"};
    out.push_back(
        {keys.count(s) ? Kind::keyword : Kind::identifier, s, line, column});
    return;
  }
  if (std::isdigit((unsigned char)c)) {
    while (current_ < source_.size() &&
           (std::isdigit((unsigned char)source_[current_]) ||
            source_[current_] == '.'))
      advance();
    out.push_back({Kind::number, source_.substr(start_, current_ - start_),
                   line, column});
    return;
  }
  if (c == '"' || c == '\'') {
    char quote = c;
    while (current_ < source_.size() && source_[current_] != quote) {
      if (source_[current_] == '\\' && current_ + 1 < source_.size())
        advance();
      advance();
    }
    if (current_ < source_.size())
      advance();
    out.push_back({Kind::string,
                   source_.substr(start_ + 1, current_ - start_ - 2), line,
                   column});
    return;
  }
  if ((c == '=' || c == '!' || c == '<' || c == '>') && match('=')) {
  }
  out.push_back(
      {Kind::symbol, source_.substr(start_, current_ - start_), line, column});
}
std::vector<Token> Lexer::scan() {
  std::vector<Token> out;
  while (current_ < source_.size())
    scan_token(out);
  out.push_back({Kind::end, "", line_, column_});
  return out;
}
}
