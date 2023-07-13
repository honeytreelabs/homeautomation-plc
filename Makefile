# note: parallel Make not supported for this Makefile

all: build-native

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
		make \
		ninja-build \
		python3 \
		python-is-python3 \
		python3-pip \
		valgrind

### local development (non-optimized binaries with debug symbols)

.PHONY: prepare-native
prepare-native: export sourcedir=$(mkfile_path)
prepare-native:
	mkdir -p build.native
	cd build.native \
		&& if [ -x "${sourcedir}/vcpkg/vcpkg" ]; then export PATH="${PATH}:${sourcedir}/vcpkg"; else ${sourcedir}/vcpkg/bootstrap-vcpkg.sh; fi \
		&& cmake \
			-DCMAKE_BUILD_TYPE=Debug \
			-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
			-DCMAKE_TOOLCHAIN_FILE=$(sourcedir)/vcpkg/scripts/buildsystems/vcpkg.cmake \
			-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=$(sourcedir)/cmake/toolchain/toolchain-native.cmake \
			-DENABLE_COVERAGE=True \
			-GNinja $(sourcedir)
	-ln -sf build.native/compile_commands.json $(sourcedir)

.PHONY: native
build-native: sourcedir=$(mkfile_path)
build-native:
	$(MAKE) -f $(sourcedir)/Makefile prepare-native sourcedir=$(sourcedir)
	cd build.native \
		&& ninja -v

.PHONY: test
test: export LUA_PATH=$(mkfile_path)/deps/luaunit/?.lua;$(mkfile_path)/src/runtime/library/?.lua
test: testdir=build.native
test:
	ctest -j $$(nproc) --test-dir $(testdir) --verbose -E 'mqtt_test|mqtt_memchecked_test'
	ctest --test-dir $(testdir) --verbose -R 'mqtt_test|mqtt_memchecked_test'

.PHONY: test-lua
test-lua: export LUA_PATH=$(mkfile_path)/deps/luaunit/?.lua;$(mkfile_path)/src/runtime/library/?.lua
test-lua:
	lua5.4 $(mkfile_path)/src/runtime/library/test/test_lua_library.lua

.PHONY: coverage
coverage: sourcedir=$(mkfile_path)
coverage: testdir=build.native
coverage:
	mkdir -p coverage
	gcovr --verbose --root $(sourcedir) --exclude deps --exclude examples --html-nested $(testdir)/coverage/coverage.html $(testdir)

.PHONY: test-nomemcheck
test-nomemcheck: export LUA_PATH=/usr/share/lua/5.4/?.lua
test-nomemcheck: testdir=build.native
test-nomemcheck:
	ctest --test-dir $(testdir) --verbose -E '.*_memchecked_.*'

.PHONY: test-failed
test-failed: export LUA_PATH=/usr/share/lua/5.4/?.lua
test-failed: testdir=build.native
test-failed:
	ctest --test-dir $(testdir) --verbose --rerun-failed --output-on-failure

### Raspberry Pi ports (optimized binaries)

.PHONY: prepare-generic
prepare-generic: sourcedir=$(mkfile_path)
prepare-generic:
	mkdir -p build.$(name)
	cd build.$(name) \
		&& if [ -x "${sourcedir}/vcpkg/vcpkg" ]; then export PATH="${PATH}:${sourcedir}/vcpkg"; else ${sourcedir}/vcpkg/bootstrap-vcpkg.sh; fi \
		&& cmake \
			-DCMAKE_BUILD_TYPE=RelMinSize \
			-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
			-DCMAKE_TOOLCHAIN_FILE=$(sourcedir)/vcpkg/scripts/buildsystems/vcpkg.cmake \
			-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=$(sourcedir)/cmake/toolchain/toolchain-$(toolchain).cmake \
			-DVCPKG_TARGET_TRIPLET=$(triplet) \
			-GNinja $(sourcedir)

.PHONY: build-generic
build-generic: sourcedir=$(mkfile_path)
build-generic:
	$(MAKE) -f $(sourcedir)/Makefile prepare-generic name=$(name) triplet=$(triplet) toolchain=$(toolchain) sourcedir=$(sourcedir)
	cd build.$(name) \
		&& ninja -v

.PHONY: prepare-rpi3
prepare-rpi3: sourcedir=$(mkfile_path)
prepare-rpi3:
	$(MAKE) -f $(sourcedir)/Makefile prepare-generic name=rpi3 triplet=arm64-linux toolchain=aarch64-rpi3 sourcedir=$(sourcedir)

.PHONY: build-rpi3
build-rpi3: sourcedir=$(mkfile_path)
build-rpi3:
	$(MAKE) -f $(sourcedir)/Makefile build-generic sourcedir=$(sourcedir) name=rpi3 triplet=arm64-linux toolchain=aarch64-rpi3

.PHONY: rpi2-prepare
rpi2-prepare: sourcedir=$(mkfile_path)
rpi2-prepare:
	$(MAKE) -f $(sourcedir)/Makefile prepare-generic name=rpi2 triplet=arm-linux toolchain=armv7hf-rpi2 sourcedir=$(sourcedir)

.PHONY: build-rpi2
build-rpi2: sourcedir=$(mkfile_path)
build-rpi2:
	$(MAKE) -f $(sourcedir)/Makefile build-generic sourcedir=$(sourcedir) name=rpi2 triplet=arm-linux toolchain=armv7hf-rpi2

.PHONY: clean
clean:
	rm -rf build.native build.rpi3 build.rpi2
