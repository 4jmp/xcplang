#include "xcp/module/module_loader.hpp"
#include <fstream>
#include <sstream>
namespace xcp::module { std::filesystem::path ModuleLoader::resolve(const std::filesystem::path&p)const{if(p.extension()==".xcp")return p;return p.string()+".xcp";} int ModuleLoader::execute(const std::filesystem::path&p){auto file=resolve(p);std::ifstream in(file);if(!in)throw std::runtime_error("cannot import "+file.string());std::stringstream text;text<<in.rdbuf();return xcp::run_source(text.str(),file.string());} }
