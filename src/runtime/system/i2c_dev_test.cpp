#include <i2c_dev.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

TEST_CASE("i2c dev: PCF8574 output test") {
  auto pcf8574 = HomeAutomation::IO::I2C::PCF8574Output(0x20);

  // IO mock
  class : public HomeAutomation::IO::I2C::IReadWrite {
  public:
    virtual void read(std::uint8_t address,
                      HomeAutomation::IO::ByteSpan data) override {
      callCntRead++;
      REQUIRE(address == 0x20);
      REQUIRE(data.size_bytes() == 1);
      data[0] = 0x55; // will be inverted by PCF8574Output => 0xaa
    }

    virtual void write(std::uint8_t address,
                       HomeAutomation::IO::ByteSpan const bytes) override {
      callCntWrite++;
      REQUIRE(address == 0x20);
      REQUIRE(bytes.size_bytes() == 1);
      REQUIRE(bytes[0] == 0x54); // has already been inverted by PCF8574Output
    }
    std::size_t callCntRead = 0;
    std::size_t callCntWrite = 0;
  } io;

  pcf8574.init(&io);
  // outputs now have state 0xaa
  pcf8574.setOutput(0, true); // output states => 0xab
  pcf8574.write(&io);
  pcf8574.write(&io); // must not invoke io.write()

  REQUIRE(io.callCntRead == 1);
  REQUIRE(io.callCntWrite == 1);
}
