#pragma once
#include "xcp/vm/bytecode.hpp"
#include <vector>
namespace xcp::vm { class VM { public: Value execute(const Bytecode& chunk); private: std::vector<Value> stack_; Value pop(); }; }
