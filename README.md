# HomeAutomation PLC

## General Description and Features

This SoftPLC framework allows for developing applications using the cyclic execution pattern typically found in PLCs. The guiding principle is to provide all that's needed for the application logic (such as IO states) as variable values. Application developers can therefore fully focus on creating the actual application logic without having to deal with low-level system details.

All that's required to be passed to the resulting binary is a configuration in YAML format. This file may even contain the PLC application logic in Lua (optional).

Example for two port expanders attached to the I<sup>2</sup>C bus of a Raspberry Pi:

``` yaml
tasks:
  - name: main
    interval: 25000  # us
    programs:
      - name: BlindLogic
        type: Lua
        script: |
          function Init(gv) BLIND = Blind.new(BlindConfigFromMillis(500, 50000, 50000)) end
          function Cycle(gv, now)
            gv.outputs.blind_up, gv.outputs.blind_down =
              BLIND:execute(now, gv.inputs.button_up, gv.inputs.button_down)
          end
    io:
      - type: i2c
        bus: /dev/i2c-1
        components:
          0x3b:  # i2c address
            type: pcf8574
            direction: input
            inputs:
              0: button_up
              1: button_down
          0x20:  # i2c address
            type: max7311
            direction: output
            outputs:
              0: blind_up
              1: blind_down
```

Also see the introductory article on our website (more to come):
- https://honeytreelabs.com/posts/smart-home-requirements-and-architecture/

Supported featuers:

- Scheduler supporting an arbitrary number of tasks.
- Implementation of programs in C/C++ and Lua.
- I<sup>2</sup>C, ModBus, MQTT IO variables.
- A small standard library containing useful and tested logic blocks.
- A comprehensive build framework with multitude of tests.
- A statically linked binary with batteries included.

## Getting started

Requirements:

- [GNU Make](https://www.gnu.org/software/make/)
- [Docker Compose](https://docs.docker.com/compose/install/) (test environment)
- [Python 3](https://www.python.org/)
- [CMake 3.20+](https://cmake.org/)
- [Ninja](https://ninja-build.org/)
- [GCC 8+](https://gcc.gnu.org/)
- [Valgrind](https://valgrind.org/)

The Conan package manager will be installed in a [Python 3 venv](https://docs.python.org/3/library/venv.html).

Currently, the following platforms are currently supported:

- Native (most likely this is x86_64)
- Raspberry Pi 4 (use Raspberry Pi 3 target)
- Raspberry Pi 3
- Raspberry Pi 2

Preparation (Debian):

``` shell
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
    python3-venv \
    valgrind
```

Building:

- `make conan-install`: install the conan package manager
- `make conan-install-deps-native`: build and install dependencies for native platform
- `make conan-install-deps-rpi3`: build and install dependencies for Raspberry Pi 3 (and 4)
- `make conan-install-deps-rpi2`: build and install dependencies for Raspberry Pi 2
- `make native`: build tests for native platform
- `make test`: build and run tests locally directly on the build platform
- `make rpi3`: build for Raspberry Pi 3 (and 4) platform
- `make rpi2`: build for Raspberry Pi 2 platform

## Roadmap

The following list is a living document mentioning what is planned in the future.

### IEC 61131-3 Programs

In central Europe, PLCs used in typical industrial automation scenarios are programmed according to the IEC 61131-3 standard. Therefore, on the middle or long run, we want to implement an interpreter which is capable of executing such logic.

### Integration of a Prometheus Client

For better analysis of what happened when and how often, we want to integrate a Prometheus Client, such as [prometheus-cpp](https://github.com/jupp0r/prometheus-cpp).

### Improved Testing Capabilities

For inlined Lua application logic I want to implement a testing framework that allows for making formally sure that the contained application behaves as expected.

### Command and Control Interface

This interface allows for Starting/Stoping/Pausing the runtime dependening on commands received via different channels, such as MQTT and/or HTTP push mechanisms.

### Global Variables with Logic

Changes in the values of global variable values (= events) invoke callbacks that can be registered with them:

- Variable value changed.
- Get all variable values.

### time_points/durations as simple 64-bit values

This allows for easier interaction with interpreters and conversion between different time bases.

## References

I wrote a couple of articles about the protocols mentioned in this project. Unfortunately, they are in German. But most likely, it is possible to translate them with some available online tools:

- [Feldbussysteme unter Linux konfigurieren und einsetzen](https://www.linux-magazin.de/ausgaben/2018/04/feldbusse/), Rainer Poisel, Linux Magazin 04/2018
- [Hausautomatisierung auf Basis des MQTT-Protokolls](https://www.linux-magazin.de/ausgaben/2017/07/mqtt/), Rainer Poisel, Linux Magazin 07/2017
- [Hausautomatisierung mit I2C-Buskomponenten](https://www.linux-magazin.de/ausgaben/2016/12/i2c-bus/), Rainer Poisel, Linux Magazin 12/2016
