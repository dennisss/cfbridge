

.PHONY: build

build:
	mkdir -p build
	cd build; cmake ..; make

run:
	./build/bridge ${CONFIG}

clean:
	rm -rf build
