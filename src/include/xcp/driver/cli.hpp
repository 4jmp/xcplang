#pragma once
#include <string>
#include <vector>
namespace xcp::driver { struct Options { enum class Mode{run,repl,help,version}; Mode mode=Mode::help; std::string file; }; Options parse_args(const std::vector<std::string>& args); int execute(const Options& options); }
