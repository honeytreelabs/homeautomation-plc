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
		python3-venv \
		valgrind

.PHONY: conan-install
conan-install:
	if [ ! -e build.venv ]; then python3 -m venv build.venv; fi
	(cd build.venv \
		&& . bin/activate \
		&& pip install conan \
		&&	if ! conan profile show default > /dev/null 2>&1; then \
				conan profile new default --detect; \
			fi \
		&& conan profile update settings.build_type=Debug default \
		&& conan profile update settings.compiler.libcxx=libstdc++11 default \
		&& cp $(mkfile_path)/conan/rpi3.profile ~/.conan/profiles/rpi3 \
		&& cp $(mkfile_path)/conan/rpi2.profile ~/.conan/profiles/rpi2 \
		&& cp $(mkfile_path)/conan/build.profile ~/.conan/profiles/build)

.PHONY: conan-install-deps
conan-install-deps:
	if ! [ -d build.deps ]; then mkdir build.deps; fi
	-find deps -regex '.*test_package/build$$' -type d -exec rm -rf "{}" \;
	. build.venv/bin/activate \
		&& conan create --profile:build=build --profile:host=$(profile) deps/libmodbus \
		&& conan create --profile:build=build --profile:host=$(profile) deps/zlib \
		&& conan create --profile:build=build --profile:host=$(profile) deps/openssl \
		&& conan create --profile:build=build --profile:host=$(profile) deps/paho-mqtt-c \
		&& conan create --profile:build=build --profile:host=$(profile) deps/paho-mqtt-cpp \
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
native-prepare:
	if ! [ -d build ]; then mkdir build; fi
	cd build \
		&& . ../build.venv/bin/activate \
		&& conan install .. \
		&& cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain/toolchain-native.cmake -GNinja ..
	ln -sf build/compile_commands.json

.PHONY: native
native: native-prepare
	cd build \
		&& ninja -v

.PHONY: test-up
test-up:
	$(MAKE) -C test/mosquitto up

.PHONY: test-status
test-status:
	$(MAKE) -C test/mosquitto status

.PHONY: test-down
test-down:
	$(MAKE) -C test/mosquitto down

.PHONY: test
test: export LUA_PATH=/usr/share/lua/5.4/?.lua
test: test-up
	ctest -j $$(nproc) --test-dir build --verbose

.PHONY: test-nomemcheck
test-nomemcheck: export LUA_PATH=/usr/share/lua/5.4/?.lua
test-nomemcheck:
	ctest --test-dir build --verbose -E '.*_memchecked_.*'

.PHONY: test-failed
test-failed: export LUA_PATH=/usr/share/lua/5.4/?.lua
test-failed:
	ctest --test-dir build --verbose --rerun-failed --output-on-failure -E '.*_memchecked_.*'

### Raspberry Pi ports (optimized binaries)

.PHONY: prepare-generic
prepare-generic:
	if ! [ -d build.$(name) ]; then mkdir build.$(name); fi
	cd build.$(name) \
		&& . ../build.venv/bin/activate \
		&& conan install --profile=$(profile) .. \
		&& cmake -DCMAKE_BUILD_TYPE=RelMinSize -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain/toolchain-$(toolchain).cmake -GNinja ..

.PHONY: generic
generic:
	cd build.$(name) \
		&& ninja -v

.PHONY: rpi3-prepare
rpi3-prepare: name=rpi3
rpi3-prepare: profile=rpi3
rpi3-prepare: toolchain=aarch64-rpi3
rpi3-prepare: prepare-generic

.PHONY: rpi3
rpi3: rpi3-prepare
	$(MAKE) generic name=rpi3

.PHONY: rpi2-prepare
rpi2-prepare: name=rpi2
rpi2-prepare: profile=rpi2
rpi2-prepare: toolchain=armv7hf-rpi2
rpi2-prepare: prepare-generic

.PHONY: rpi2
rpi2: rpi2-prepare
	$(MAKE) generic name=rpi2

.PHONY: clean
clean:
	rm -rf build.venv build build.rpi3 build.rpi2
