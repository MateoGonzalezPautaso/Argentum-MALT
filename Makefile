.PHONY: all test clean editor client common server build

compile-debug:
	mkdir -p build/
	cmake -S . -B ./build -DCMAKE_BUILD_TYPE=Debug -DTALLER_MAKE_WARNINGS_AS_ERRORS=OFF $(EXTRA_GENERATE)
	cmake --build build/ $(EXTRA_COMPILE)

run-tests: compile-debug
	./build/taller_tests

all: clean run-tests

clean:
	rm -Rf build/
