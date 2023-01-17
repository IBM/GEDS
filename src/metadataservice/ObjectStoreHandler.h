/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef METADATASERVICE_OBJECT_STORE_HANDLER
#define METADATASERVICE_OBJECT_STORE_HANDLER

#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>

#include <absl/status/status.h>

#include "ObjectStoreConfig.h"

class ObjectStoreHandler {
  mutable std::shared_mutex _mutex;

  auto getReadLock() const { return std::shared_lock(_mutex); }

  std::unique_lock<std::shared_mutex> getWriteLock() { return std::unique_lock(_mutex); }

  std::map<std::string, std::shared_ptr<const geds::ObjectStoreConfig>> _map;

public:
  ObjectStoreHandler();

  absl::Status insertConfig(std::shared_ptr<const geds::ObjectStoreConfig> config);
  std::vector<std::shared_ptr<const geds::ObjectStoreConfig>> listConfigs() const;
};

#endif
