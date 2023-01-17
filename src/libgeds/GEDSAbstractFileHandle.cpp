/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "GEDSAbstractFileHandle.h"

#include "GEDS.h"
#include "StatisticsCounter.h"

namespace geds::service {
std::string getLocalPath(std::shared_ptr<GEDS> geds, const std::string &bucket,
                         const std::string &key) {
  return geds->getLocalPath(bucket, key);
}

absl::Status seal(std::shared_ptr<GEDS> geds, GEDSFileHandle &fileHandle, bool update,
                  size_t size) {
  return geds->seal(fileHandle, update, size);
}
} // namespace geds::service
