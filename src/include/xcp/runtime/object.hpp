#pragma once
#include "xcp/vm/value.hpp"
#include <string>
namespace xcp::runtime { struct StringObject { std::string value; explicit StringObject(std::string text):value(std::move(text)){} }; struct ArrayObject { vm::Array values; void append(vm::Value value){values.push_back(std::move(value));} }; std::size_t array_size(const ArrayObject& array); }
