/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ObjectStoreHandler.h"

ObjectStoreHandler::ObjectStoreHandler() {}

absl::Status
ObjectStoreHandler::insertConfig(std::shared_ptr<const geds::ObjectStoreConfig> config) {
  auto lock = getWriteLock();
  auto key = config->bucket;
  auto [it, success] = _map.insert(std::make_pair(key, config));
  if (!success && *(it->second) != *config) {
    return absl::AlreadyExistsError("ObjectStore config for bucket already exists: '" + key + "'.");
  }
  return absl::OkStatus();
}
std::vector<std::shared_ptr<const geds::ObjectStoreConfig>>
ObjectStoreHandler::listConfigs() const {
  auto lock = getReadLock();
  std::vector<std::shared_ptr<const geds::ObjectStoreConfig>> result;
  for (auto it : _map) {
    result.push_back(it.second);
  }
  return result;
}
