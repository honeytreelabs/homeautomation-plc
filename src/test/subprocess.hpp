#pragma once

#include <chrono>
#include <functional>

namespace TestUtil {

int exec(const char *cmd);

bool poll_for_cond(std::function<bool()> cond,
                   std::chrono::milliseconds duration,
                   std::chrono::milliseconds wait_period);

} // namespace TestUtil
