all: native

.PHONY: conan-install
conan-install:
	if [ ! -e build.venv ]; then python -m venv build.venv; fi
	(cd build.venv \
		&& . bin/activate \
		&& pip install conan \
		&&	if ! conan profile show default > /dev/null 2>&1; then \
				conan profile new default --detect \
					&& conan profile update settings.compiler.libcxx=libstdc++11 default; \
			fi \
		&&	cp ../.conan/rpi4.profile ~/.conan/profiles/rpi4 \
		&&	cp ../.conan/rpi2.profile ~/.conan/profiles/rpi2)

.PHONY: conan-install-deps
conan-install-deps:
	if ! [ -d build.deps ]; then mkdir build.deps; fi
	. build.venv/bin/activate \
		&& conan profile update settings.compiler.libcxx=libstdc++11 $(profile) \
		&& conan create --profile=$(profile) deps/libmodbus \
		&& conan install --profile=$(profile) -if build.deps -of build.deps --build=zlib --build=openssl --build=paho-mqtt-c --build=paho-mqtt-cpp --build=lua .
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

.PHONY: native
native:
	if ! [ -d build ]; then mkdir build; fi
	cd build \
		&& . ../build.venv/bin/activate \
		&& conan install .. \
		&& cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain/toolchain-native.cmake -GNinja .. \
		&& ninja -v
	ln -sf build/compile_commands.json

.PHONY: test
test: native
	cd build && ctest --verbose

.PHONY: roof
roof:
	if ! [ -d build.roof ]; then mkdir build.roof; fi
	cd build.roof \
		&& . ../build.venv/bin/activate \
		&& conan install --profile=rpi4 .. \
		&& cmake -DCMAKE_BUILD_TYPE=RelMinSize -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain/toolchain-aarch64-rpi4.cmake -GNinja .. \
		&& ninja -v bin/roof
	ln -sf build.root/compile_commands.json

.PHONY: deploy-roof
deploy-roof: roof
	scp -O build.roof/bin/roof root@raspberry-d.lan:/opt

.PHONY: ground
ground:
	if ! [ -d build.ground ]; then mkdir build.ground; fi
	cd build.ground \
		&& . ../build.venv/bin/activate \
		&& conan install --profile=rpi2 .. \
		&& cmake -DCMAKE_BUILD_TYPE=RelMinSize -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain/toolchain-armv7hf-rpi2.cmake -GNinja .. \
		&& ninja -v bin/ground
	ln -sf build.ground/compile_commands.json

.PHONY: deploy-ground
deploy-ground: ground
	scp -O build.ground/bin/ground root@raspberry-o.lan:/opt

.PHONY: clean
clean:
	rm -rf build.venv build build.roof build.ground

.PHONY: basement
basement:
	if ! [ -d build.basement ]; then mkdir build.basement; fi
	cd build.basement \
		&& . ../build.venv/bin/activate \
		&& conan install --profile=rpi4 .. \
		&& cmake -DCMAKE_BUILD_TYPE=RelMinSize -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain/toolchain-aarch64-rpi4.cmake -GNinja .. \
		&& ninja -v bin/basement
	ln -sf build.basement/compile_commands.json
