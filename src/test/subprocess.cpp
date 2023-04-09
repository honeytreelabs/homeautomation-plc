#include <subprocess.hpp>

#include <array>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <thread>

namespace TestUtil {

int exec(const char *cmd) {
  std::array<char, 128> buffer;
  auto pipe = popen(cmd, "r");

  if (!pipe)
    throw std::runtime_error("popen() failed!");

  while (!feof(pipe)) {
    fgets(buffer.data(), buffer.size(), pipe);
  }

  return pclose(pipe);
}

bool poll_for_cond(std::function<bool()> cond, std::size_t tries,
                   std::chrono::milliseconds wait_period) {
  for (std::size_t cnt = tries; cnt > 0; --cnt) {
    auto result = cond();
    if (result) {
      return true;
    }
    std::this_thread::sleep_for(wait_period);
  }
  return false;
}

} // namespace TestUtil
