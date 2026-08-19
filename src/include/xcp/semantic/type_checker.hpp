#pragma once
#include "xcp/parser/ast.hpp"
#include <string>
namespace xcp::semantic {
class TypeChecker {
public:
  void check(const ast::Program &program) const;
};
} // namespace xcp::semantic
