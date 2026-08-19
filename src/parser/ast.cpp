#include "xcp/parser/ast.hpp"
namespace xcp::ast { std::string dump_program(const Program&p){std::string out;for(const auto&s:p.statements){if(!out.empty())out+='\n';out+=s?s->dump():"<invalid>";}return out;} }
