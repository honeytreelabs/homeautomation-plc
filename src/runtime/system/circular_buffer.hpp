#pragma once

// shamelessly taken from
// https://github.com/embeddedartistry/embedded-resources/

#include <array>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>

namespace HomeAutomation {

template <class T, size_t TElemCount> class circular_buffer {
public:
  explicit circular_buffer() = default;

  void put(T item) noexcept {
    std::lock_guard<std::mutex> lock(mutex);

    buf[head] = item;

    if (buffer_full) {
      tail = (tail + 1) % TElemCount;
    }

    head = (head + 1) % TElemCount;

    buffer_full = head == tail;

    cv.notify_one();
  }

  std::optional<T> get() const noexcept {
    std::lock_guard<std::mutex> lock(mutex);

    if (empty_unsafe()) {
      return std::nullopt;
    }

    // Read data and advance the tail (we now have a free space)
    auto val = buf[tail];
    buffer_full = false;
    tail = (tail + 1) % TElemCount;

    return val;
  }

  std::optional<T> peek_for(
      std::chrono::duration<int, std::milli> const &rel_time) const noexcept {
    using namespace std::chrono_literals;

    std::unique_lock<std::mutex> lock(mutex);
    if (!cv.wait_for(lock, rel_time, [this] { return !empty_unsafe(); })) {
      return std::nullopt;
    }

    return buf[tail];
  }

  std::optional<T> get_for(
      std::chrono::duration<int, std::milli> const &rel_time) const noexcept {
    using namespace std::chrono_literals;

    std::unique_lock<std::mutex> lock(mutex);
    if (!cv.wait_for(lock, rel_time, [this] { return !empty_unsafe(); })) {
      return std::nullopt;
    }

    // Read data and advance the tail (we now have a free space)
    auto val = buf[tail];
    buffer_full = false;
    tail = (tail + 1) % TElemCount;

    return val;
  }

  void reset() noexcept {
    std::lock_guard<std::mutex> lock(mutex);
    head = tail;
    buffer_full = false;
  }

  bool empty() const noexcept {
    // Can have a race condition in a multi-threaded application
    std::lock_guard<std::mutex> lock(mutex);
    // if head and tail are equal, we are empty
    return empty_unsafe();
  }

  bool full() const noexcept {
    // If tail is ahead the head by 1, we are full
    return buffer_full;
  }

  size_t capacity() const noexcept { return TElemCount; }

  size_t size() const noexcept {
    // A lock is needed in size ot prevent a race condition, because head_,
    // tail_, and full_ can be updated between executing lines within this
    // function in a multi-threaded application
    std::lock_guard<std::mutex> lock(mutex);

    size_t size = TElemCount;

    if (!buffer_full) {
      if (head >= tail) {
        size = head - tail;
      } else {
        size = TElemCount + head - tail;
      }
    }

    return size;
  }

private:
  mutable std::mutex mutex;
  mutable std::condition_variable cv;
  mutable std::array<T, TElemCount> buf;
  mutable size_t head = 0;
  mutable size_t tail = 0;
  mutable bool buffer_full = false;

  bool empty_unsafe() const noexcept {
    return (!buffer_full && (head == tail));
  }
};

} // namespace HomeAutomation
