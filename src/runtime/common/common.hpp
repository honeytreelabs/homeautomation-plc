#pragma once

#include <algorithm>
#include <chrono>

namespace HomeAutomation {

using TimeStamp = std::chrono::time_point<std::chrono::high_resolution_clock>;

// https://ctrpeach.io/posts/cpp20-string-literal-template-parameters/
template <size_t N> struct StringLiteral {
  constexpr StringLiteral(const char (&str)[N]) { std::copy_n(str, N, value); }

  char value[N];
};

} // namespace HomeAutomation
