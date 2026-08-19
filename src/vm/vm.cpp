#include "xcp/vm/vm.hpp"
#include <iostream>
#include <stdexcept>
namespace xcp::vm {
Value VM::pop() {
  if (stack_.empty())
    throw std::runtime_error("stack underflow");
  auto v = stack_.back();
  stack_.pop_back();
  return v;
}
Value VM::execute(const Bytecode &c) {
  stack_.clear();
  for (auto i : c.code) {
    switch (i.op) {
    case Opcode::constant:
      stack_.push_back(c.constants.at(i.operand));
      break;
    case Opcode::add: {
      auto b = pop(), a = pop();
      stack_.push_back(std::get<double>(a.data) + std::get<double>(b.data));
      break;
    }
    case Opcode::subtract: {
      auto b = pop(), a = pop();
      stack_.push_back(std::get<double>(a.data) - std::get<double>(b.data));
      break;
    }
    case Opcode::multiply: {
      auto b = pop(), a = pop();
      stack_.push_back(std::get<double>(a.data) * std::get<double>(b.data));
      break;
    }
    case Opcode::divide: {
      auto b = pop(), a = pop();
      stack_.push_back(std::get<double>(a.data) / std::get<double>(b.data));
      break;
    }
    case Opcode::print:
      if (!stack_.empty())
        std::cout << pop().repr() << '\n';
      break;
    case Opcode::halt:
      return stack_.empty() ? Value{} : pop();
    }
  }
  return stack_.empty() ? Value{} : pop();
}
} // namespace xcp::vm
