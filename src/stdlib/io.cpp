#include "xcp/stdlib/io.hpp"
#include <iostream>
namespace xcp::stdlib { vm::Value print(const std::vector<vm::Value>&a){for(const auto&v:a)std::cout<<v.repr()<<' ';std::cout<<'\n';return{};} vm::Value input(){std::string s;std::getline(std::cin,s);return s;} vm::Value len(const vm::Value&v){if(auto p=std::get_if<std::string>(&v.data))return static_cast<double>(p->size());if(auto p=std::get_if<std::shared_ptr<vm::Array>>(&v.data))return static_cast<double>((*p)->size());return 0.0;} }
