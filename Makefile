all: native

.PHONY: conan-install
conan-install:
	if [ ! -e build.venv ]; then python -m venv build.venv; fi
	(cd build.venv \
		&& . bin/activate \
		&& pip install conan \
		&&	if ! conan profile show default > /dev/null 2>&1; then \
				conan profile new default --detect; \
			fi \
		&& conan profile update settings.build_type=Debug default \
		&& conan profile update settings.compiler.libcxx=libstdc++11 default \
		&&	cp ../.conan/rpi4.profile ~/.conan/profiles/rpi4 \
		&&	cp ../.conan/rpi2.profile ~/.conan/profiles/rpi2 \
		&&	cp ../.conan/build.profile ~/.conan/profiles/build)

.PHONY: conan-install-deps
conan-install-deps:
	if ! [ -d build.deps ]; then mkdir build.deps; fi
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

.PHONY: conan-install-deps-rpi4
conan-install-deps-rpi4: profile=rpi4
conan-install-deps-rpi4: conan-install-deps

.PHONY: prepare
prepare: conan-install-deps-native conan-install-deps-rpi4 conan-install-deps-rpi2

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
test: test-up
	ctest -j $$(nproc) --test-dir build --verbose

.PHONY: test-nomemcheck
test-nomemcheck:
	ctest --test-dir build --verbose -E '.*_memchecked_.*'

.PHONY: executable-prepare-generic
executable-prepare-generic:
	if ! [ -d build.$(name) ]; then mkdir build.$(name); fi
	cd build.$(name) \
		&& . ../build.venv/bin/activate \
		&& conan install --profile=$(profile) .. \
		&& cmake -DCMAKE_BUILD_TYPE=RelMinSize -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain/toolchain-$(toolchain).cmake -GNinja ..
	ln -sf build.$(name)/compile_commands.json

.PHONY: executable-generic
executable-generic:
	cd build.$(name) \
		&& ninja -v bin/$(name)

.PHONY: deploy-generic
deploy-generic:
	ssh root@$(host) /etc/init.d/homeautomation disable || true
	ssh root@$(host) /etc/init.d/homeautomation stop || true
	# grace period to make sure the process exits
	sleep 1
	scp -O build.$(name)/bin/$(name) root@$(host):/opt
	scp -O build.$(name)/src/homeautomation.$(name) root@$(host):/etc/init.d/homeautomation
	ssh root@$(host) /etc/init.d/homeautomation enable

### roof

.PHONY: roof-prepare
roof-prepare: name=roof
roof-prepare: profile=rpi4
roof-prepare: toolchain=aarch64-rpi4
roof-prepare: executable-prepare-generic

.PHONY: roof
roof: roof-prepare
	$(MAKE) executable-generic name=roof

.PHONY: deploy-roof
deploy-roof: roof
	$(MAKE) deploy-generic host=raspberry-d.lan name=roof

### ground

.PHONY: ground-prepare
ground-prepare: name=ground
ground-prepare: profile=rpi2
ground-prepare: toolchain=armv7hf-rpi2
ground-prepare: executable-prepare-generic

.PHONY: ground
ground: ground-prepare
	$(MAKE) executable-generic name=ground

.PHONY: deploy-ground
deploy-ground: ground
	$(MAKE) deploy-generic host=raspberry-o.lan name=ground

### basement
# note: needs Alpine or OpenWrt based container

.PHONY: basement-prepare
basement-prepare: name=basement
basement-prepare: profile=rpi4
basement-prepare: toolchain=aarch64-rpi4
basement-prepare: executable-prepare-generic

.PHONY: basement
basement: basement-prepare
	$(MAKE) executable-generic name=basement

# currently there is no deployment step because basement execution context is different

.PHONY: clean
clean:
	rm -rf build.venv build build.roof build.ground build.basement
