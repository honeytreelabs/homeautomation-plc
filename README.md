# homeautomation-plc

## Getting started

Requirements:

- [GNU Make](https://www.gnu.org/software/make/)
- [Python 3](https://www.python.org/)
- [CMake 3.20+](https://cmake.org/)
- [Ninja](https://ninja-build.org/)
- [GCC 8+](https://gcc.gnu.org/)
- Optional: [crosstool-ng](https://crosstool-ng.github.io/)

The Conan package manager will be installed in a [Python 3 venv](https://docs.python.org/3/library/venv.html).

The following platforms are supported:

- Native (most likely this is x86_64)
- Raspberry Pi 4
- Raspberry Pi 2

In order to build the toolchain for the latter two, please refer to the directories beneath [cmake/toolchain/ct-ng](./cmake/toolchain/ct-ng). They contain the configurations for crosstool-ng. Find the documentation for this tool [here](https://crosstool-ng.github.io/docs/).

Building:

- `make conan-install`: install the conan package manager
- `make conan-install-deps-native`: build and install dependencies for native platform
- `make conan-install-deps-rpi4`: build and install dependencies for Raspberry Pi 4
- `make conan-install-deps-rpi2`: build and install dependencies for Raspberry Pi 2
- `make native`: build tests for native platform
- `make roof`: build PLC application for the roof Raspberry Pi 4
- `make ground`: build PLC application for the ground Raspberry Pi 2
- `make basement`: build PLC application for the basement Raspberry Pi 4
- `make test`: build and run tests lokally directly on the build platform
