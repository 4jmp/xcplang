#include "xcp/vm/value.hpp"
#include <sstream>
namespace xcp::vm {
std::string Value::repr() const {
  if (std::holds_alternative<std::monostate>(data))
    return "null";
  if (auto p = std::get_if<double>(&data)) {
    std::ostringstream o;
    o << *p;
    return o.str();
  }
  if (auto p = std::get_if<bool>(&data))
    return *p ? "true" : "false";
  if (auto p = std::get_if<std::string>(&data))
    return *p;
  auto a = std::get<std::shared_ptr<Array>>(data);
  std::string r = "[";
  for (size_t i = 0; i < a->size(); ++i) {
    if (i)
      r += ", ";
    r += (*a)[i].repr();
  }
  return r + "]";
}
bool Value::truthy() const {
  if (auto p = std::get_if<bool>(&data))
    return *p;
  if (auto p = std::get_if<double>(&data))
    return *p != 0;
  if (auto p = std::get_if<std::string>(&data))
    return !p->empty();
  return !std::holds_alternative<std::monostate>(data);
}
} // namespace xcp::vm
