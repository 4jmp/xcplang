#pragma once
#include "xcp/runtime/object.hpp"
#include <memory>
#include <vector>
#include <utility>
namespace xcp::runtime { class GC { public: template<class T,class... A> std::shared_ptr<T> make(A&&...a){auto p=std::make_shared<T>(std::forward<A>(a)...);objects_.push_back(p);return p;} std::size_t object_count()const{return objects_.size();} void collect(); private: std::vector<std::shared_ptr<void>> objects_; }; }
