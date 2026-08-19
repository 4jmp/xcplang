#pragma once
#include <memory>
#include <string>
#include <variant>
#include <vector>
namespace xcp::vm {
struct Value;
using Array = std::vector<Value>;
using ValueData = std::variant<std::monostate, double, bool, std::string,
                               std::shared_ptr<Array>>;
struct Value {
  ValueData data;
  Value() = default;
  template <class T> Value(T v) : data(std::move(v)) {}
  std::string repr() const;
  bool truthy() const;
};
} // namespace xcp::vm
