#include "xcp/driver/cli.hpp"
#include "xcp/module/module_loader.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
namespace xcp::driver { Options parse_args(const std::vector<std::string>&a){Options o;if(a.empty()||a[0]=="help"||a[0]=="--help")return o;if(a[0]=="--version"){o.mode=Options::Mode::version;return o;}if(a[0]=="repl"){o.mode=Options::Mode::repl;return o;}o.mode=Options::Mode::run;o.file=a[0]=="run"?(a.size()>1?a[1]:""):a[0];return o;} int execute(const Options&o){if(o.mode==Options::Mode::help){std::cout<<"xcp <file.xcp> | xcp run <file.xcp> | xcp repl\n";return 0;}if(o.mode==Options::Mode::version){std::cout<<"xcplang 1.0.0\n";return 0;}if(o.mode==Options::Mode::repl){std::string s;while(std::cout<<"> "&&std::getline(std::cin,s))xcp::run_source(s,"<repl>");return 0;}if(o.file.size()<4||o.file.substr(o.file.size()-4)!=".xcp"){std::cerr<<"xcp error: files must use .xcp\n";return 2;}return module::ModuleLoader{}.execute(o.file);} }
