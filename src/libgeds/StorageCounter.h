/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstddef>

#include "RWConcurrentObjectAdaptor.h"

namespace geds {
struct StorageCounter : public utility::RWConcurrentObjectAdaptor {
  size_t allocated{0};
  size_t used{0};
  size_t free{0};

  StorageCounter(size_t allocated) : allocated(allocated) {}

  void updateUsed(size_t used) {
    auto lock = getWriteLock();
    this->used = used;
    this->free = (used > allocated) ? 0 : allocated - used;
  }

  void updateAllocated(size_t allocated) {
    auto lock = getWriteLock();
    this->allocated = allocated;
    this->free = (used > allocated) ? 0 : allocated - used;
  }
};
} // namespace geds
