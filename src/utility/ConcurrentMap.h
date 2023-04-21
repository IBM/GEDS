/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <shared_mutex>

#include "RWConcurrentObjectAdaptor.h"

namespace utility {

template <class K, class V, typename L = std::less<K>>
class ConcurrentMap : RWConcurrentObjectAdaptor {
  std::map<K, V, L> _map;

public:
  std::optional<V> get(const K &key) const {
    auto lock = getReadLock();
    auto loc = _map.find(key);
    if (loc != _map.end()) {
      return loc->second;
    }
    return std::nullopt;
  }

  V insertOrExists(const K &key, V value) {
    auto lock = getWriteLock();
    auto it = _map.find(key);
    if (it == _map.end()) {
      it = _map.insert(it, {key, value});
    }
    return it->second;
  }

  void insertOrReplace(const K &key, V value) {
    auto lock = getWriteLock();
    _map[key] = value;
  }

  bool remove(const K &key) {
    auto lock = getWriteLock();
    auto it = _map.find(key);
    if (it == _map.end()) {
      return false;
    }
    _map.erase(it);
    return true;
  }

  int size() {
    auto lock = getReadLock();
    return _map.size();
  }

  void clear() {
    auto lock = getWriteLock();
    _map.clear();
  }

  std::optional<V> getAndRemove(const K &key) {
    auto lock = getWriteLock();
    auto it = _map.find(key);
    if (it == _map.end()) {
      return std::nullopt;
    }
    auto result = it->second;
    _map.erase(it);
    return result;
  }

  bool exists(const K &key) {
    auto lock = getReadLock();
    return _map.find(key) != _map.end();
  }

  template <typename Probe> void removeRange(const Probe &probe) {
    auto lock = getWriteLock();
    auto [probeStart, probeEnd] = _map.equal_range(probe);
    if (probeStart != _map.end()) {
      _map.erase(probeStart, probeEnd);
    }
  }

  void remove(std::function<bool(const K &, V &)> selector) {
    auto lock = getWriteLock();
    auto items = std::vector<K>();
    for (auto it = _map.begin(); it != _map.end();) {
      it = selector(it->first, it->second) ? _map.erase(it) : std::next(it);
    }
  }

  bool removeIf(const K &key, std::function<bool(const V &)> selector) {
    auto lock = getWriteLock();
    auto it = _map.find(key);
    if (it == _map.end()) {
      return false;
    }
    auto result = it->second;
    if (selector(result)) {
      _map.erase(it);
      return true;
    }
    return false;
  }

  void forall(std::function<void(V &)> action) const {
    auto lock = getReadLock();
    for (auto it : _map) {
      action(it.second);
    }
  }

  void forall(std::function<void(const K &, V &)> action) const {
    auto lock = getReadLock();
    for (auto it : _map) {
      action(it.first, it.second);
    }
  }
};

} // namespace utility
