.PHONY: build run deb clean

build:
	mkdir -p build
	cd build && cmake .. && $(MAKE)

run: build
	./build/bin/kubsh

deb: build
	dpkg-buildpackage -us -uc -b -tc
	mv ../kubsh*.deb .

clean:
	rm -rf build *.deb *.changes *.buildinfo
