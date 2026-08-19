#pragma once
#include <cstddef>
#include <string>
namespace xcp::lexer {
enum class Kind { identifier, number, string, keyword, symbol, end };
struct Token { Kind kind{Kind::end}; std::string lexeme; std::size_t line{1}; std::size_t column{1}; };
const char* kind_name(Kind kind);
}
