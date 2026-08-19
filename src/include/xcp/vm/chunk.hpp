#pragma once
#include "xcp/vm/bytecode.hpp"
#include <string>
namespace xcp::vm {
class Chunk {
public:
  void constant(Value value);
  void instruction(Opcode op, std::uint16_t operand = 0);
  const Bytecode &bytecode() const { return code_; }
  std::string disassemble() const;

private:
  Bytecode code_;
};
} // namespace xcp::vm
