#include "xcp/stdlib/math.hpp"
#include <cmath>
namespace xcp::stdlib {
vm::Value absolute(const vm::Value &v) {
  return std::abs(std::get<double>(v.data));
}
} // namespace xcp::stdlib
