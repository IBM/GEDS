/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "S3ObjectStores.h"
#include "Logging.h"
#include "S3Endpoint.h"

namespace geds::s3 {

absl::Status ObjectStores::registerStore(const std::string &label, std::string endpointUrl,
                                         std::string accessKey, std::string secretKey) {
  LOG_INFO("Registering object store: ", label, " with ", endpointUrl);
  auto lock = getWriteLock();
  auto exists = _map.find(label);
  if (exists != _map.end()) {
    auto endpoint = exists->second;
    if (endpoint->_accessKey == accessKey && endpoint->_endpointUrl == endpointUrl) {
      return absl::OkStatus();
    }
    return absl::AlreadyExistsError(label + " already exists with different values!");
  }
  auto obj = std::make_shared<Endpoint>(std::move(endpointUrl), std::move(accessKey),
                                        std::move(secretKey));
  _map[label] = obj;
  return absl::OkStatus();
}

bool ObjectStores::isRegistered(const std::string &label) const {
  auto lock = getReadLock();
  auto loc = _map.find(label);
  return loc != _map.end();
}

absl::StatusOr<std::shared_ptr<Endpoint>> ObjectStores::get(const std::string &label) const {
  auto lock = getReadLock();
  auto loc = _map.find(label);
  if (loc == _map.end()) {
    return absl::NotFoundError(label + " is not registered!");
  }
  return loc->second;
}
} // namespace geds::s3
