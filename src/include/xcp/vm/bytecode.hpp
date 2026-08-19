#pragma once
#include "xcp/vm/opcode.hpp"
#include "xcp/vm/value.hpp"
#include <cstdint>
#include <vector>
namespace xcp::vm { struct Instruction { Opcode op; std::uint16_t operand{0}; }; struct Bytecode { std::vector<Value> constants; std::vector<Instruction> code; std::size_t add_constant(Value v){constants.push_back(std::move(v));return constants.size()-1;} void emit(Opcode op,std::uint16_t arg=0){code.push_back({op,arg});} }; }
