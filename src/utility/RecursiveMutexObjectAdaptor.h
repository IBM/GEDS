/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#pragma once

#include <mutex>

namespace utility {

/**
 * @brief Concurrent object adaptor for read/write objects. Non-recursive.
 */
class RecursiveMutexObjectAdaptor {
  mutable std::recursive_mutex __mutex;

protected:
  auto lock() { return std::lock_guard<std::recursive_mutex>(__mutex); }
};

} // namespace utility
