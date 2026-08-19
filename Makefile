CXX ?= c++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra
TARGET=xcp
SRC=src/driver/main.cpp src/driver/cli.cpp src/module/module_loader.cpp src/stdlib/io.cpp src/stdlib/math.cpp src/runtime/gc.cpp src/runtime/object.cpp src/vm/value.cpp src/vm/vm.cpp src/vm/chunk.cpp src/semantic/scope.cpp src/semantic/type_checker.cpp src/parser/parser.cpp src/parser/ast.cpp src/lexer/lexer.cpp src/lib/runtime.cpp
all: $(TARGET)
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -Isrc/include $(SRC) -o $@
test: $(TARGET)
	./xcp tests/hello.xcp
	./xcp tests/advanced.xcp
	./xcp tests/imports.xcp
	./xcp tests/prefixes.xcp
	./xcp tests/discord_config.xcp
	./xcp tests/helpers.xcp
clean:
	rm -f $(TARGET)
.PHONY: all test clean
