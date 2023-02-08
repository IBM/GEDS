/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "KVSBucket.h"

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "Logging.h"
#include "Object.h"
#include "Path.h"

KVSBucket::KVSBucket(const std::string &name) : _name(name) {}

KVSBucket::~KVSBucket() {}

absl::StatusOr<std::shared_ptr<KVSBucket::Container>> KVSBucket::getObject(const std::string &key) {
  auto lock = getReadLock();
  auto it = _map.find(utility::Path{key});
  if (it == _map.end()) {
    return absl::NotFoundError("Key " + key + " does not exist.");
  }
  return it->second;
}

absl::Status KVSBucket::createObject(const geds::Object &obj) {
  auto lock = getWriteLock();
  if (_map.find(utility::Path{obj.id.key}) != _map.end()) {
    LOG_DEBUG("Overwriting ", obj.id.key, " since it already exists!");
  }
  _map[utility::Path{obj.id.key}] = std::make_shared<KVSBucket::Container>(obj.info);
  return absl::OkStatus();
}

absl::Status KVSBucket::updateObject(const geds::Object &obj) {
  auto data = getObject(obj.id.key);
  if (!data.ok()) {
    return data.status();
  }
  auto container = data.value();
  auto lock = container->getWriteLock();
  container->obj = obj.info;
  return absl::OkStatus();
}

absl::Status KVSBucket::deleteObject(const std::string &key) {
  auto lock = getWriteLock();
  auto it = _map.find(utility::Path{key});
  if (it == _map.end()) {
    return absl::NotFoundError("Key " + key + " does not exist.");
  }
  _map.erase(it);
  return absl::OkStatus();
}

absl::Status KVSBucket::deleteObjectPrefix(const std::string &prefix) {
  auto lock = getWriteLock();
  auto [prefixStart, prefixEnd] = utility::prefixSearch(_map, prefix);
  if (prefixStart == _map.end()) {
    return absl::NotFoundError("No objects starting with " + prefix + " found.");
  }
  _map.erase(prefixStart, prefixEnd);

  return absl::OkStatus();
}

absl::StatusOr<geds::Object> KVSBucket::lookup(const std::string &key) {
  auto data = getObject(key);
  if (!data.ok()) {
    return data.status();
  }
  auto container = data.value();
  auto lock = container->getReadLock();
  return geds::Object{geds::ObjectID{_name, key}, container->obj};
}

absl::StatusOr<std::pair<std::vector<geds::Object>, std::vector<std::string>>>
KVSBucket::listObjects(const std::string &keyPrefix, char delimiter) {
  auto lock = getReadLock();
  auto result = std::vector<geds::Object>();
  const auto prefixLength = keyPrefix.length();

  std::set<std::string> commonPrefixes;
  auto [prefixStart, prefixEnd] = utility::prefixSearch(_map, keyPrefix);
  for (auto it = prefixStart; it != prefixEnd; it++) {
    if (delimiter != 0) {
      const auto &path = it->first.name;
      auto loc = path.find(delimiter, prefixLength);
      if (loc != std::string::npos) {
        // Also include delimitor in string. This makes it compatible with AWS:
        // https://docs.aws.amazon.com/AmazonS3/latest/API/API_CommonPrefix.html
        commonPrefixes.insert(path.substr(0, loc + 1));
        continue;
      }
    }
    auto container = it->second;
    result.push_back(geds::Object{geds::ObjectID{_name, it->first.name}, container->obj});
  }
  return std::make_pair(std::move(result),
                        std::vector<std::string>{commonPrefixes.begin(), commonPrefixes.end()});
}
