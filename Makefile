# note: parallel Make not supported for this Makefile

all: native

### dependency handling

mkfile_path := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

.PHONY: deps-install
deps-install:
	if ! grep -q 'ID=.*\(debian\|ubuntu\)' /etc/os-release; then \
		echo "Only debian/ubuntu Linux are supported as build platform"; \
		exit 1; \
	fi
	if ! command -v sudo; then echo "sudo must be available."; exit 1; fi
	sudo apt update && sudo apt install -y \
		build-essential \
		cmake \
		g++ \
		g++-aarch64-linux-gnu \
		g++-arm-linux-gnueabihf \
		lua-unit \
		make \
		ninja-build \
		python3 \
		python-is-python3 \
		python3-pip \
		valgrind

.PHONY: conan-install
conan-install:
	if ! conan profile show default > /dev/null 2>&1; then \
			conan profile new default --detect; \
	fi \
		&& conan profile update settings.build_type=Debug default \
		&& conan profile update settings.compiler.libcxx=libstdc++11 default \
		&& cp $(mkfile_path)/conan/rpi3.profile ~/.conan/profiles/rpi3 \
		&& cp $(mkfile_path)/conan/rpi2.profile ~/.conan/profiles/rpi2 \
		&& cp $(mkfile_path)/conan/build.profile ~/.conan/profiles/build

.PHONY: conan-install-deps
conan-install-deps:
	if ! [ -d build.deps ]; then mkdir build.deps; fi
	-find deps -regex '.*test_package/build$$' -type d -exec rm -rf "{}" \;
	conan create --profile:build=build --profile:host=$(profile) $(mkfile_path)/deps/libmodbus \
		&& conan create --profile:build=build --profile:host=$(profile) $(mkfile_path)/deps/zlib \
		&& conan create --profile:build=build --profile:host=$(profile) $(mkfile_path)/deps/openssl \
		&& conan create --profile:build=build --profile:host=$(profile) $(mkfile_path)/deps/paho-mqtt-c \
		&& conan create --profile:build=build --profile:host=$(profile) $(mkfile_path)/deps/paho-mqtt-cpp \
		&& conan install --profile:build=build --profile:host=$(profile) -if build.deps -of build.deps --build=lua .
	rm -rf build.deps

.PHONY: conan-install-deps-native
conan-install-deps-native: profile=default
conan-install-deps-native: conan-install-deps

.PHONY: conan-install-deps-rpi2
conan-install-deps-rpi2: profile=rpi2
conan-install-deps-rpi2: conan-install-deps

.PHONY: conan-install-deps-rpi3
conan-install-deps-rpi3: profile=rpi3
conan-install-deps-rpi3: conan-install-deps

.PHONY: prepare-all-deps
prepare: conan-install-deps-native conan-install-deps-rpi3 conan-install-deps-rpi2

### local development (non-optimized binaries with debug symbols)

.PHONY: native-prepare
native-prepare: sourcedir=..
native-prepare:
	if ! [ -d build ]; then mkdir build; fi
	cd build \
		&& conan install $(mkfile_path) \
		&& cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=$(sourcedir)/cmake/toolchain/toolchain-native.cmake -GNinja $(sourcedir)
	-ln -sf build/compile_commands.json $(sourcedir)

.PHONY: native
native: sourcedir=..
native:
	$(MAKE) -f $(sourcedir)/Makefile native-prepare sourcedir=$(sourcedir)
	cd build \
		&& ninja -v

.PHONY: test
test: export LUA_PATH=/usr/share/lua/5.4/?.lua
test: testdir=build
test:
	ctest -j $$(nproc) --test-dir $(testdir) --verbose

.PHONY: test-nomemcheck
test-nomemcheck: export LUA_PATH=/usr/share/lua/5.4/?.lua
test-nomemcheck: testdir=build
test-nomemcheck:
	ctest --test-dir $(testdir) --verbose -E '.*_memchecked_.*'

.PHONY: test-failed
test-failed: export LUA_PATH=/usr/share/lua/5.4/?.lua
test-failed: testdir=build
test-failed:
	ctest --test-dir $(testdir) --verbose --rerun-failed --output-on-failure -E '.*_memchecked_.*'

### Raspberry Pi ports (optimized binaries)

.PHONY: prepare-generic
prepare-generic:
	if ! [ -d build.$(name) ]; then mkdir build.$(name); fi
	cd build.$(name) \
		&& conan install --profile=$(profile) $(sourcedir) \
		&& cmake -DCMAKE_BUILD_TYPE=RelMinSize -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=$(sourcedir)/cmake/toolchain/toolchain-$(toolchain).cmake -GNinja $(sourcedir)

.PHONY: generic
generic: sourcedir=$(mkfile_path)
generic:
	$(MAKE) -f $(sourcedir)/Makefile prepare-generic name=$(name) profile=$(profile) toolchain=$(toolchain) sourcedir=$(sourcedir)
	cd build.$(name) \
		&& ninja -v

.PHONY: rpi3-prepare
rpi3-prepare: sourcedir=$(mkfile_path)
rpi3-prepare:
	$(MAKE) -f $(sourcedir)/Makefile prepare-generic name=rpi3 profile=rpi3 toolchain=aarch64-rpi3 sourcedir=$(sourcedir)

.PHONY: rpi3
rpi3: sourcedir=$(mkfile_path)
rpi3:
	$(MAKE) -f $(sourcedir)/Makefile generic sourcedir=$(sourcedir) name=rpi3 profile=rpi3 toolchain=aarch64-rpi3

.PHONY: rpi2-prepare
rpi2-prepare: sourcedir=$(mkfile_path)
rpi2-prepare:
	$(MAKE) -f $(sourcedir)/Makefile prepare-generic name=rpi2 profile=rpi2 toolchain=armv7hf-rpi2 sourcedir=$(sourcedir)

.PHONY: rpi2
rpi2: sourcedir=$(mkfile_path)
rpi2:
	$(MAKE) -f $(sourcedir)/Makefile generic sourcedir=$(sourcedir) name=rpi2 profile=rpi2 toolchain=armv7hf-rpi2

.PHONY: clean
clean:
	rm -rf build build.rpi3 build.rpi2
