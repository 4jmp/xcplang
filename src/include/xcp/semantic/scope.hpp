#pragma once
#include "xcp/vm/value.hpp"
#include <memory>
#include <string>
#include <unordered_map>
namespace xcp::semantic { class Scope { public: explicit Scope(std::shared_ptr<Scope> parent={}); void define(std::string name,vm::Value value); bool assign(const std::string& name,vm::Value value); vm::Value lookup(const std::string& name)const; private: std::shared_ptr<Scope> parent_; std::unordered_map<std::string,vm::Value> values_; }; }
