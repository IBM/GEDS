/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "GEDSAbstractFileHandle.h"

#include "GEDS.h"

namespace geds::service {
std::string getLocalPath(std::shared_ptr<GEDS> geds, const std::string &bucket,
                         const std::string &key) {
  return geds->getLocalPath(bucket, key);
}

absl::Status seal(std::shared_ptr<GEDS> geds, GEDSFileHandle &fileHandle, bool update,
                  size_t size) {
  return geds->seal(fileHandle, update, size);
}

absl::StatusOr<std::shared_ptr<geds::s3::Endpoint>> getS3Endpoint(std::shared_ptr<GEDS> geds,
                                                                  const std::string &bucket) {
  return geds->getS3Endpoint(bucket);
}

} // namespace geds::service
