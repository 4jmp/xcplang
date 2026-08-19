#pragma once
#include "xcp/runtime.hpp"
#include <filesystem>
#include <string>
namespace xcp::module { class ModuleLoader { public: int execute(const std::filesystem::path& path); private: std::filesystem::path resolve(const std::filesystem::path&)const; }; }
