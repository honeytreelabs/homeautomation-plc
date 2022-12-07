# HomeAutomation PLC

## General Description and Features

This Soft PLC software provides a framework to implement home automation logic in the cyclic execution pattern typically found in PLCs. The idea is to have some execution environment with minimal dependencies to the underlying operating system (Linux for now).

Supported featuers:

- Scheduler which supports one resource containing multiple tasks which in turn may execute one or more programs.
- I2C blocks
- Blocks representing arbitrary instances of Lua interpreters.
- Logic blocks representing window blinds, light switches and low-level logic such as triggers.
- A comprehensive build framework with multitude of tests including memory checks to make sure this piece of software also runs for longer periods of time.

## Getting started

Requirements:

- [GNU Make](https://www.gnu.org/software/make/)
- [Docker Compose](https://docs.docker.com/compose/install/)
- [Python 3](https://www.python.org/)
- [CMake 3.20+](https://cmake.org/)
- [Ninja](https://ninja-build.org/)
- [GCC 8+](https://gcc.gnu.org/)
- [Valgrind](https://valgrind.org/)
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
- `make conan-install-deps-rpi4`: build and install dependencies for Raspberry Pi 4 (optional)
- `make conan-install-deps-rpi2`: build and install dependencies for Raspberry Pi 2 (optional)
- `make native`: build tests for native platform
- `make roof`: build PLC application for the roof Raspberry Pi 4 (optional)
- `make ground`: build PLC application for the ground Raspberry Pi 2 (optional)
- `make basement`: build PLC application for the basement Raspberry Pi 4 (optional)
- `make test`: build and run tests locally directly on the build platform

## Roadmap

### MQTT Blocks (WiP)

This would enable the implementation of high-level workflows using, for example, [Node-RED](https://nodered.org/) or other solutions capable of logically linking MQTT messages.

For me this would also allow for inter-floor communication in my house: press a push button in the ground floor and turn on a light in the upper floor.

### Modbus Blocks

In the electric cabinet of the basement of my house, components are wired together using Modbus. Having Modbus blocks would allow for implementing the logic of my basement installation.

### IEC 61131-3 Parser

In central Europe, PLCs used in typical industrial automation scenarios are programmed according to the IEC 61131-3 standard. Therefore, on the middle or long run, I want to implement an interpreter which is capable of executing such logic.

## References

I wrote a couple of articles about the protocols mentioned in this project. Unfortunately, they are in German. But most likely, it is possible to translate them with some available online tools:

- [Feldbussysteme unter Linux konfigurieren und einsetzen](https://www.linux-magazin.de/ausgaben/2018/04/feldbusse/), Rainer Poisel, Linux Magazin 04/2018
- [Hausautomatisierung auf Basis des MQTT-Protokolls](https://www.linux-magazin.de/ausgaben/2017/07/mqtt/,) Rainer Poisel, Linux Magazin 07/2017
- [Hausautomatisierung mit I2C-Buskomponenten](https://www.linux-magazin.de/ausgaben/2016/12/i2c-bus/,) Rainer Poisel, Linux Magazin 12/2016
