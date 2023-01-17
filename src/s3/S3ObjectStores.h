/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GEDS_S3_OBJECT_STORES_H
#define GEDS_S3_OBJECT_STORES_H

#include <map>
#include <memory>
#include <shared_mutex>

#include <absl/status/status.h>
#include <absl/status/statusor.h>

#include "ObjectStoreConfig.h"
#include "RWConcurrentObjectAdaptor.h"
#include "S3Endpoint.h"

namespace geds::s3 {
class ObjectStores : utility::RWConcurrentObjectAdaptor {
  std::map<std::string, std::shared_ptr<Endpoint>> _map;

public:
  ObjectStores() = default;
  ~ObjectStores() = default;
  ObjectStores &operator=(const ObjectStores &) = delete;

  absl::Status registerStore(const std::string &label, std::string endpointUrl,
                             std::string accessKey, std::string secretKey);

  bool isRegistered(const std::string &label) const;
  absl::StatusOr<std::shared_ptr<Endpoint>> get(const std::string &label) const;
};
} // namespace geds::s3

#endif
