#include "xcp/semantic/type_checker.hpp"
#include <stdexcept>
namespace xcp::semantic {
void TypeChecker::check(const ast::Program &p) const {
  for (const auto &s : p.statements)
    if (!s)
      throw std::runtime_error("invalid statement");
}
} // namespace xcp::semantic
