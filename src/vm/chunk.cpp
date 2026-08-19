#include "xcp/vm/chunk.hpp"
namespace xcp::vm {
void Chunk::constant(Value v) {
  code_.emit(Opcode::constant,
             static_cast<std::uint16_t>(code_.add_constant(std::move(v))));
}
void Chunk::instruction(Opcode o, std::uint16_t a) { code_.emit(o, a); }
std::string Chunk::disassemble() const {
  return "xcplang bytecode (" + std::to_string(code_.code.size()) +
         " instructions)";
}
} // namespace xcp::vm
