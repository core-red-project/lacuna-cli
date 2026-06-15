.PHONY: all configure build format clean install test

all: build

configure:
	cmake -B build -DCMAKE_BUILD_TYPE=Release

build: configure
	cmake --build build --config Release

format:
	find src/ -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i

clean:
	rm -rf build

test: build
	bash tests/integration_test.sh

