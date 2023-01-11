/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#pragma once

#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <shared_mutex>

#include "RWConcurrentObjectAdaptor.h"

namespace utility {

template <class K> class ConcurrentQueue : RWConcurrentObjectAdaptor {
  std::queue<K> _queue;

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

  bool empty() const {
    auto lock = getReadLock();
    return _queue.empty();
  }

  void emplace(K k) {
    auto lock = getWriteLock();
    _queue.emplace(k);
  }

  void emplace(K &&k) {
    auto lock = getWriteLock();
    _queue.emplace(k);
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
