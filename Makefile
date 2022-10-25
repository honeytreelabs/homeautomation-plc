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
	if [ ! -e build ]; then mkdir build; fi
	cd build \
		&& . ../build.venv/bin/activate \
		&& conan profile update settings.compiler.libcxx=libstdc++11 default \
		&& conan install --build=zlib --build=openssl --build=paho-mqtt-c --build=paho-mqtt-cpp ..

.PHONY: native
native:
	if [ ! -e build ]; then mkdir build; fi
	cd build \
		&& cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain/toolchain-native.cmake -GNinja .. \
		&& ninja -v
	ln -sf build/compile_commands.json

.PHONY: test
test: native
	cd build && ctest --verbose

roof-conan: prepare
	if [ ! -e build.roof ]; then mkdir build.roof; fi
	cd build.roof \
		&& . ../build.venv/bin/activate \
		&& conan install --profile=rpi4 --build=zlib --build=openssl --build=paho-mqtt-c --build=paho-mqtt-cpp ..

.PHONY: roof
roof:
	if [ ! -e build.roof ]; then mkdir build.roof; fi
	cd build.roof \
		&& cmake -DCMAKE_BUILD_TYPE=RelMinSize -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain/toolchain-aarch64-rpi4.cmake -GNinja .. \
		&& ninja -v bin/roof

.PHONY: deploy-roof
deploy-roof: roof
	scp -O build.roof/bin/roof root@raspberry-d.lan:/opt

ground-conan: prepare
	if [ ! -e build.ground ]; then mkdir build.ground; fi
	cd build.ground \
		&& . ../build.venv/bin/activate \
		&& conan install --profile=rpi2 --build=zlib --build=openssl --build=paho-mqtt-c --build=paho-mqtt-cpp ..

.PHONY: ground
ground:
	if [ ! -e build.ground ]; then mkdir build.ground; fi
	cd build.ground \
		&& cmake -DCMAKE_BUILD_TYPE=RelMinSize -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain/toolchain-armv7hf-rpi2.cmake -GNinja .. \
		&& ninja -v bin/ground

.PHONY: deploy-ground
deploy-ground: ground
	scp -O build.ground/bin/ground root@raspberry-o.lan:/opt

.PHONY: clean
clean:
	rm -rf build.venv build build.rasppi
