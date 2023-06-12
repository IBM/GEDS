/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <utility>
#include <vector>

#include <absl/status/status.h>
#include <absl/status/statusor.h>

#include "Object.h"
#include "Path.h"
#include "RWConcurrentObjectAdaptor.h"

class MDSKVSBucket : public utility::RWConcurrentObjectAdaptor {
  class Container : public utility::RWConcurrentObjectAdaptor {
  public:
    geds::ObjectInfo obj;
    Container(geds::ObjectInfo &&objArg) : obj(std::move(objArg)) {}
    Container(const geds::ObjectInfo objArg) : obj(std::move(objArg)) {}

    Container(const MDSKVSBucket &) = delete;
  };

  std::map<utility::Path, std::shared_ptr<Container>, std::less<>> _map;
  absl::StatusOr<std::shared_ptr<Container>> getObject(const std::string &key);

  std::string _name;

public:
  MDSKVSBucket(const std::string &name);

  ~MDSKVSBucket() = default;

  absl::Status createObject(const geds::Object &obj);

  absl::Status updateObject(const geds::Object &obj);

  absl::Status deleteObject(const std::string &key);
  absl::Status deleteObjectPrefix(const std::string &prefix);

  absl::StatusOr<geds::Object> lookup(const std::string &key);
  absl::StatusOr<std::pair<std::vector<geds::Object>, std::vector<std::string>>>
  listObjects(const std::string &keyPrefix, char delimiter = 0);

  void forall(std::function<void(const utility::Path &, const geds::ObjectInfo &)> action) const;
};
