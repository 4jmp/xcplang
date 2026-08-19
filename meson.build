project('xcplang','cpp',version:'1.0.0',default_options:['cpp_std=c++20','warning_level=2'])
xcp_inc=include_directories('src/include')
executable('xcp', files('src/driver/main.cpp','src/driver/cli.cpp','src/module/module_loader.cpp','src/stdlib/io.cpp','src/stdlib/math.cpp','src/runtime/gc.cpp','src/runtime/object.cpp','src/vm/value.cpp','src/vm/vm.cpp','src/vm/chunk.cpp','src/semantic/scope.cpp','src/semantic/type_checker.cpp','src/parser/parser.cpp','src/parser/ast.cpp','src/lexer/lexer.cpp','src/lib/runtime.cpp'), include_directories:xcp_inc, install:true)
