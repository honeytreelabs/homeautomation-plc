all: native

.PHONY: prepare
prepare:
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

# TODO define dependencies between native and native-conan
native-conan: prepare
	if ! [ -d build ]; then mkdir build; fi
	cd build \
		&& . ../build.venv/bin/activate \
		&& conan profile update settings.compiler.libcxx=libstdc++11 default \
		&& conan create ../deps/libmodbus \
		&& conan install ..

.PHONY: native
native:
	if ! [ -d build ]; then mkdir build; fi
	cd build \
		&& cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain/toolchain-native.cmake -GNinja .. \
		&& ninja -v
	ln -sf build/compile_commands.json

.PHONY: test
test: native
	cd build && ctest --verbose

roof-conan: prepare
	if ! [ -d build.roof ]; then mkdir build.roof; fi
	cd build.roof \
		&& . ../build.venv/bin/activate \
		&& conan create --profile=rpi4 ../deps/libmodbus \
		&& conan install --profile=rpi4 ..

.PHONY: roof
roof:
	if ! [ -d build.roof ]; then mkdir build.roof; fi
	cd build.roof \
		&& cmake -DCMAKE_BUILD_TYPE=RelMinSize -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain/toolchain-aarch64-rpi4.cmake -GNinja .. \
		&& ninja -v bin/roof

.PHONY: deploy-roof
deploy-roof: roof
	scp -O build.roof/bin/roof root@raspberry-d.lan:/opt

ground-conan: prepare
	if ! [ -d build.ground ]; then mkdir build.ground; fi
	cd build.ground \
		&& . ../build.venv/bin/activate \
		&& conan create --profile=rpi2 ../deps/libmodbus \
		&& conan install --profile=rpi2 ..

.PHONY: ground
ground:
	if ! [ -d build.ground ]; then mkdir build.ground; fi
	cd build.ground \
		&& cmake -DCMAKE_BUILD_TYPE=RelMinSize -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain/toolchain-armv7hf-rpi2.cmake -GNinja .. \
		&& ninja -v bin/ground

.PHONY: deploy-ground
deploy-ground: ground
	scp -O build.ground/bin/ground root@raspberry-o.lan:/opt

.PHONY: clean
clean:
	rm -rf build.venv build build.roof build.ground

.PHONY: basement-conan
basement-conan:
	if ! [ -d build.basement ]; then mkdir build.basement; fi
	cd build.basement \
		&& . ../build.venv/bin/activate \
		&& conan create --profile=rpi4 ../deps/libmodbus \
		&& conan install --profile=rpi4 ..

.PHONY: basement
basement:
	if ! [ -d build.basement ]; then mkdir build.basement; fi
	cd build.basement \
		&& cmake -DCMAKE_BUILD_TYPE=RelMinSize -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain/toolchain-aarch64-rpi4.cmake -GNinja .. \
		&& ninja -v bin/basement
