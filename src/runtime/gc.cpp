#include "xcp/runtime/gc.hpp"
#include <algorithm>
namespace xcp::runtime { void GC::collect(){objects_.erase(std::remove_if(objects_.begin(),objects_.end(),[](const auto&p){return p.use_count()==1;}),objects_.end());} }
