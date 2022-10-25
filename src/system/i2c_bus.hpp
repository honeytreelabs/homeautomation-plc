#pragma once

#include <spdlog/spdlog.h>

#include <linux/i2c-dev.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace HomeAutomation {
namespace IO {

using ByteSpan = std::span<std::uint8_t>;

namespace I2C {

class IRead {
public:
  virtual ~IRead() {}
  virtual void read(std::uint8_t address, ByteSpan data) = 0;
};

class IWrite {
public:
  virtual ~IWrite() {}
  virtual void write(std::uint8_t address, ByteSpan const bytes) = 0;
};

class IReadWrite : public IRead, public IWrite {
public:
  virtual ~IReadWrite() {}
};

class InputModule {
public:
  virtual ~InputModule() = default;
  virtual void init(IReadWrite *ireadwrite) = 0;
  virtual void read(IReadWrite *iread) = 0;
};

class OutputModule {
public:
  virtual ~OutputModule() = default;
  virtual void init(IReadWrite *io) = 0;
  virtual void write(IReadWrite *io) = 0;
};

class Bus : public IReadWrite {
public:
  Bus(std::string &path) : path(path), inputs{}, outputs{} {}

  void RegisterInput(InputModule *module) { inputs.push_back(module); }
  void RegisterOutput(OutputModule *module) { outputs.push_back(module); }

  void readInputs() {
    for (auto input : inputs) {
      input->read(this);
    }
  }
  void writeOutputs() {
    for (auto output : outputs) {
      output->write(this);
    }
  }

  void init() {
    open();

    for (auto input : inputs) {
      input->init(this);
    }
    for (auto output : outputs) {
      output->init(this);
    }
  }

  virtual void open() = 0;
  virtual void close() = 0;

  virtual void setAddress(std::uint8_t address) = 0;

protected:
  std::string path;

private:
  std::vector<InputModule *> inputs;
  std::vector<OutputModule *> outputs;
};

// https://github.com/Digilent/linux-userspace-examples/blob/master/i2c_example_linux/src/i2c.c
class RealBus : public Bus {
public:
  RealBus(std::string path) : Bus(path), fd{} {}

  void open() override {
    // open handle for this bus
    fd = ::open(path.c_str(), O_RDWR);
    if (fd < 0) {
      spdlog::critical("Failed to open i2c bus {}", path);
      throw std::invalid_argument(std::string("failed to open: ") + path);
    }
  }

  void read(std::uint8_t address, ByteSpan data) override {
    setAddress(address);

    // perform physical I/O
    auto rc = ::read(fd, data.data(), data.size());
    if (rc < 0) {
      spdlog::critical("Could not read from i2c bus.");
      // throw exception
    }
  }

  void write(std::uint8_t address, ByteSpan const bytes) override {
    setAddress(address);

    // perform physical I/O
    auto rc = ::write(fd, bytes.data(), bytes.size());
    if (rc < 0) {
      spdlog::critical("Could not write to i2c bus.");
      // throw exception
    }
  }

  void close() override { ::close(fd); }

  void setAddress(std::uint8_t address) override {
    auto rc = ::ioctl(fd, I2C_SLAVE, address);
    if (rc < 0) {
      spdlog::critical("Cannot set address {0:x} on i2c bus", address);
      throw std::runtime_error(std::string("failed to set I2C address: ") +
                               std::to_string(address));
    }
  }

private:
  int fd;
};

class StubBus : public Bus {
public:
  StubBus(std::string path) : Bus(path) {}

  void open() override { spdlog::info("open()"); }

  void read(std::uint8_t address, ByteSpan data) override {
    setAddress(address);

    // perform physical I/O
    spdlog::info("reading {} bytes", data.size());
  }

  void write(std::uint8_t address, ByteSpan const bytes) override {
    setAddress(address);

    // perform physical I/O
    spdlog::info("writing {} bytes", bytes.size());
  }

  void close() override { spdlog::info("close()"); }

  void setAddress(std::uint8_t address) override {
    spdlog::info("setting address: {}", address);
  }
};

} // namespace I2C
} // namespace IO
} // namespace HomeAutomation
