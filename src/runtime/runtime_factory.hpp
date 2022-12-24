#pragma once

#include <gv_factory.hpp>
#include <runtime.hpp>
#include <scheduler.hpp>

#include <filesystem>
#include <map>
#include <memory>

namespace HomeAutomation {
namespace Runtime {

class RuntimeFactory {
public:
  static std::shared_ptr<Runtime> fromFile(std::filesystem::path const &path);
  static std::shared_ptr<Runtime> fromString(std::string const &str);

private:
  RuntimeFactory() = delete;
};

} // namespace Runtime
} // namespace HomeAutomation
