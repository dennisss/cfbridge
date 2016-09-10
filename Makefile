

.PHONY: build

build:
	mkdir -p build
	cd build; cmake ..; make

run:
	./build/bridge

clean:
	rm -rf build
