target_host=armv7-rpi2-linux-musleabihf
standalone_toolchain=/home/rpoisel/x-tools/armv7-rpi2-linux-musleabihf
cc_compiler=gcc
cxx_compiler=g++

[settings]
os=Linux
arch=armv7hf
compiler=gcc
compiler.version=12
compiler.libcxx=libstdc++11
build_type=Release

[env]
CONAN_CMAKE_FIND_ROOT_PATH=$standalone_toolchain/$target_host
CONAN_CMAKE_SYSROOT=$standalone_toolchain/$target_host/sysroot
PATH=[$standalone_toolchain/bin]
CHOST=$target_host
AR=$target_host-ar
AS=$target_host-as
RANLIB=$target_host-ranlib
LD=$target_host-ld
STRIP=$target_host-strip
CC=$target_host-$cc_compiler
CXX=$target_host-$cxx_compiler