#pragma once
#include "xcp/vm/value.hpp"
#include <vector>
namespace xcp::stdlib { vm::Value print(const std::vector<vm::Value>& args); vm::Value input(); vm::Value len(const vm::Value& value); }
