#pragma once
#include <string>
namespace xcp {
int run_source(const std::string &source,
               const std::string &filename = "<input>");
}
namespace xcp {
int run_source(const std::string &source, const std::string &filename,
               bool allow_destructive);
}
