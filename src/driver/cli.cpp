#include "xcp/driver/cli.hpp"
#include "xcp/module/module_loader.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
namespace xcp::driver {
Options parse_args(const std::vector<std::string> &args) {
  Options options;
  std::vector<std::string> positional;
  for (const auto &arg : args) {
    if (arg == "--allow-destructive" || arg == "-d") {
      options.allow_destructive = true;
    } else {
      positional.push_back(arg);
    }
  }
  if (positional.empty() || positional[0] == "help" ||
      positional[0] == "--help")
    return options;
  if (positional[0] == "--version" || positional[0] == "-v") {
    options.mode = Options::Mode::version;
    return options;
  }
  if (positional[0] == "repl") {
    options.mode = Options::Mode::repl;
    return options;
  }
  options.mode = Options::Mode::run;
  options.file = positional[0] == "run"
                     ? (positional.size() > 1 ? positional[1] : "")
                     : positional[0];
  return options;
}

int execute(const Options &options) {
  if (options.mode == Options::Mode::help) {
    std::cout << "xcp [--allow-destructive] <file.xcp> | xcp run <file.xcp> | "
                 "xcp repl\n";
    return 0;
  }
  if (options.mode == Options::Mode::version) {
    std::cout << "xcplang 1.0.1\n";
    return 0;
  }
  if (options.mode == Options::Mode::repl) {
    std::string source;
    while (std::cout << "> " && std::getline(std::cin, source)) {
      xcp::run_source(source, "<repl>", options.allow_destructive);
    }
    return 0;
  }
  if (options.file.size() < 4 ||
      options.file.substr(options.file.size() - 4) != ".xcp") {
    std::cerr << "xcp error: files must use .xcp\n";
    return 2;
  }
  return module::ModuleLoader{}.execute(options.file,
                                        options.allow_destructive);
}
}
