#pragma once

#include <gv_factory.hpp>
#include <scheduler.hpp>

#include <filesystem>
#include <map>
#include <memory>

namespace HomeAutomation {
namespace Runtime {

class RuntimeFactory {
public:
  static void fromString(std::string const &str, HomeAutomation::GV *gv,
                         HomeAutomation::Runtime::Scheduler *scheduler);
  static void fromFile(std::filesystem::path const &path,
                       HomeAutomation::GV *gv,
                       HomeAutomation::Runtime::Scheduler *scheduler);

private:
  RuntimeFactory() = delete;
};

} // namespace Runtime
} // namespace HomeAutomation
