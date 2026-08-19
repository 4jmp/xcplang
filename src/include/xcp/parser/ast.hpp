#pragma once
#include <memory>
#include <string>
#include <vector>
namespace xcp::ast {
struct Expr { virtual ~Expr()=default; virtual std::string dump() const=0; };
using ExprPtr=std::unique_ptr<Expr>;
struct Literal final:Expr { std::string value; explicit Literal(std::string v):value(std::move(v)){} std::string dump()const override{return value;} };
struct Name final:Expr { std::string value; explicit Name(std::string v):value(std::move(v)){} std::string dump()const override{return value;} };
struct Binary final:Expr { ExprPtr left; std::string op; ExprPtr right; Binary(ExprPtr l,std::string o,ExprPtr r):left(std::move(l)),op(std::move(o)),right(std::move(r)){} std::string dump()const override{return "("+left->dump()+" "+op+" "+right->dump()+")";} };
struct Statement { virtual ~Statement()=default; virtual std::string dump()const=0; };
using StatementPtr=std::unique_ptr<Statement>;
struct Let final:Statement { std::string name; ExprPtr value; Let(std::string n,ExprPtr v):name(std::move(n)),value(std::move(v)){} std::string dump()const override{return "let "+name+" = "+value->dump();} };
struct Program { std::vector<StatementPtr> statements; };
std::string dump_program(const Program& program);
}
