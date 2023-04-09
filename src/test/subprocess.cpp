#include <subprocess.hpp>

#include <array>
#include <cstdio>
#include <stdexcept>
#include <string>

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

} // namespace TestUtil
