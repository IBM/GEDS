/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GEDS_KVSBUCKET_H
#define GEDS_KVSBUCKET_H

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <utility>
#include <vector>

#include <absl/status/statusor.h>

#include "Common.h"
#include "Object.h"
#include "Path.h"

class KVSBucket {
  std::shared_mutex _mutex;

  auto getReadLock() { return std::shared_lock<std::shared_mutex>(_mutex); }

  auto getWriteLock() { return std::unique_lock<std::shared_mutex>(_mutex); }

  class Container {
    std::shared_mutex _m;

  public:
    auto getReadLock() { return std::shared_lock<std::shared_mutex>(_m); }

    auto getWriteLock() { return std::unique_lock<std::shared_mutex>(_m); }
    geds::ObjectInfo obj;
    Container(geds::ObjectInfo &&objArg) : obj(std::move(objArg)) {}
    Container(const geds::ObjectInfo objArg) : obj(std::move(objArg)) {}

    Container(const KVSBucket &) = delete;
  };

  std::map<utility::Path, std::shared_ptr<Container>, std::less<>> _map;
  absl::StatusOr<std::shared_ptr<Container>> getObject(const std::string &key);

  std::string _name;

public:
  KVSBucket(const std::string &name);

  ~KVSBucket();

  absl::Status createObject(const geds::Object &obj);

  absl::Status updateObject(const geds::Object &obj);

  absl::Status deleteObject(const std::string &key);
  absl::Status deleteObjectPrefix(const std::string &prefix);

  absl::StatusOr<geds::Object> lookup(const std::string &key);
  absl::StatusOr<std::pair<std::vector<geds::Object>, std::vector<std::string>>>
  listObjects(const std::string &keyPrefix, char delimiter = 0);
};

#endif // GEDS_KVSBUCKET_H
