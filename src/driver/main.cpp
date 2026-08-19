#include "xcp/driver/cli.hpp"
int main(int argc,char**argv){std::vector<std::string> args;for(int i=1;i<argc;++i)args.emplace_back(argv[i]);return xcp::driver::execute(xcp::driver::parse_args(args));}
