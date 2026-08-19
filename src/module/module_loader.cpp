#include "xcp/module/module_loader.hpp"
#include <fstream>
#include <sstream>
namespace xcp::module {
std::filesystem::path
ModuleLoader::resolve(const std::filesystem::path &path) const {
  if (path.extension() == ".xcp")
    return path;
  return path.string() + ".xcp";
}

int ModuleLoader::execute(const std::filesystem::path &path,
                          bool allow_destructive) {
  auto file = resolve(path);
  std::ifstream in(file);
  if (!in)
    throw std::runtime_error("cannot import " + file.string());
  std::stringstream source;
  source << in.rdbuf();
  return xcp::run_source(source.str(), file.string(), allow_destructive);
}
} // namespace xcp::module
