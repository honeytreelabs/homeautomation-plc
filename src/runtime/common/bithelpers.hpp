#pragma once

#include <cstddef>
#include <cstdint>

namespace HomeAutomation {
namespace BitHelpers {

template <typename T> T bitflip(T pattern) { return static_cast<T>(~pattern); }

template <typename T> T bitflip(T pattern, T mask) { return pattern ^ mask; }

template <typename T> T bitflipPos(T pattern, size_t pos) {
  return static_cast<T>((pattern ^ (1 << pos)));
}

template <typename T> T bitmask(T pattern, T mask) { return pattern & mask; }

template <typename T> bool bitget(T pattern, uint8_t pos) {
  return pattern & (1 << pos);
}

template <typename T> T bitset(T pattern, uint8_t pos, bool value) {
  if (value) {
    return pattern | (1 << pos);
  }
  return pattern & ~(1 << pos);
}

} // namespace BitHelpers
} // namespace HomeAutomation
