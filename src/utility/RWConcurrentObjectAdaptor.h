/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#pragma once

#include <mutex>
#include <shared_mutex>

namespace utility {

/**
 * @brief Concurrent object adaptor for read/write objects. Non-recursive.
 */
class RWConcurrentObjectAdaptor {
  mutable std::shared_mutex __mutex;

protected:
  auto getReadLock() const { return std::shared_lock<std::shared_mutex>(__mutex); }
  auto getWriteLock() const { return std::unique_lock<std::shared_mutex>(__mutex); }
};

} // namespace utility
