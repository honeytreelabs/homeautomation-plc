#pragma once

#include <yaml-cpp/yaml.h>

#include <string>

namespace HomeAutomation {

class Config {
public:
  static Config fromFile(std::string const &path) {
    Config result;

    YAML::Node config = YAML::LoadFile(path);
    result.mqtt.address = config["mqtt"]["address"].as<std::string>();
    result.mqtt.clientID = config["mqtt"]["client_id"].as<std::string>();
    result.mqtt.username = config["mqtt"]["username"].as<std::string>();
    result.mqtt.password = config["mqtt"]["password"].as<std::string>();
    result.i2c.bus = config["i2c"]["bus"].as<std::string>();

    return result;
  }
  struct {
    std::string address;
    std::string clientID;
    std::string username;
    std::string password;
  } mqtt;

  struct {
    std::string bus;
  } i2c;

  virtual ~Config() {}

private:
  Config() {}
  Config(Config const &);
  Config &operator=(Config const &);
};

} // namespace HomeAutomation
