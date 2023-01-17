/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>
#include <mutex>
#include <optional>
#include <set>
#include <shared_mutex>

#include "RWConcurrentObjectAdaptor.h"

namespace utility {

template <class K, typename L = std::less<K>> class ConcurrentSet : RWConcurrentObjectAdaptor {
  std::set<K, L> _set;

public:
  bool exists(const K &key) const {
    auto lock = getReadLock();
    auto loc = _set.find(key);
    return loc != _set.end();
  }

  void insert(const K &key) {
    auto lock = getWriteLock();
    _set.insert(key);
  }

  void remove(const K &key) {
    auto lock = getWriteLock();
    _set.erase(key);
  }
};

} // namespace utility
