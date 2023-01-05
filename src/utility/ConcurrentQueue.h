/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <shared_mutex>

using namespace std::chrono_literals;

#include "RWConcurrentObjectAdaptor.h"

namespace utility {

template <class K> class ConcurrentQueue : RWConcurrentObjectAdaptor {
  std::queue<K> _queue;

  std::condition_variable _cv;
  std::mutex _cv_lock;

public:
  std::optional<K> front() const {
    auto lock = getReadLock();
    if (_queue.empty()) {
      return std::nullopt;
    }
    return _queue.front();
  }

  std::optional<K> pop() {
    auto lock = getWriteLock();
    if (_queue.empty()) {
      return std::nullopt;
    }
    auto result = _queue.front();
    _queue.pop();
    return result;
  }

  K pop_wait_until_available() {
    std::optional<K> result = pop();
    if (result.has_value()) {
      return *result;
    }
    do {
      std::unique_lock<std::mutex> cv_lock(_cv_lock);
      _cv.wait_for(cv_lock, 500ms, [] { return true; });
      result = pop();
    } while (!result.has_value());
    return *result;
  }

  bool empty() const {
    auto lock = getReadLock();
    return _queue.empty();
  }

  void push(K &k) {
    auto lock = getWriteLock();
    _queue.push(k);
    _cv.notify_one();
  }

  void emplace(K &&k) {
    auto lock = getWriteLock();
    _queue.emplace(k);
    _cv.notify_one();
  }

  void remove(std::function<bool(K &)> selector) {
    auto lock = getWriteLock();
    auto items = std::vector<K>();
    for (auto it = _queue.begin(); it != _queue.end();) {
      it = selector(*it) ? _queue.erase(it) : std::next(it);
    }
  }

  //   bool exists(const K &key) const {
  //     auto lock = getReadLock();
  //     auto loc = _set.find(key);
  //     return loc != _set.end();
  //   }

  //   void insert(const K &key) {
  //     auto lock = getWriteLock();
  //     _set.insert(key);
  //   }

  //   void remove(const K &key) {
  //     auto lock = getWriteLock();
  //     _set.erase(key);
  //   }
};

} // namespace utility
