/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mutex>

#include <absl/status/status.h>

#include "KVS.h"
#include "Object.h"

#define QUOTE(str) #str
#define STRINGIFY(str) QUOTE(str)

KVS::KVS() {
#if defined(HAVE_DEFAULT_BUCKET) && HAVE_DEFAULT_BUCKET
  (void)createBucket(STRINGIFY(DEFAULT_BUCKET_NAME));
#endif
}

KVS::~KVS() {}

absl::StatusOr<std::shared_ptr<KVSBucket>> KVS::getBucket(const std::string &bucket) {
  auto lock = getReadLock();
  auto it = _map.find(bucket);
  if (it == _map.end()) {
    return absl::NotFoundError("Bucket " + bucket + " does not exist.");
  }
  return it->second;
}

absl::StatusOr<std::shared_ptr<KVSBucket>> KVS::getBucket(const geds::ObjectID &id) {
  return getBucket(id.bucket);
}

absl::Status KVS::createBucket(const std::string &bucket) {
  auto lock = getWriteLock();
  if (_map.find(bucket) != _map.end()) {
    return absl::AlreadyExistsError("Bucket " + bucket + " already exists.");
  }
  _map[bucket] = std::make_shared<KVSBucket>(bucket);
  return absl::OkStatus();
}

absl::Status KVS::deleteBucket(const std::string &bucket) {
  auto lock = getWriteLock();
  auto it = _map.find(bucket);
  if (it == _map.end()) {
    return absl::NotFoundError("Bucket " + bucket + " does not exist.");
  }
  _map.erase(it);
  return absl::OkStatus();
}

absl::StatusOr<std::vector<std::string>> KVS::listBuckets() {
  auto lock = getReadLock();
  std::vector<std::string> result;
  result.reserve(_map.size());
  for (const auto &item : _map) {
    result.push_back(item.first);
  }
  return result;
}

absl::Status KVS::bucketStatus(const std::string &bucket) {
  auto lock = getReadLock();
  if (_map.find(bucket) != _map.end()) {
    return absl::OkStatus();
  }
  return absl::NotFoundError("Bucket " + bucket + " does not exist.");
}

absl::Status KVS::createObject(const geds::Object &obj) {
  auto bucket = getBucket(obj.id);
  if (!bucket.ok()) {
    return bucket.status();
  }
  return bucket.value()->createObject(obj);
}

absl::Status KVS::updateObject(const geds::Object &obj) {
  auto bucket = getBucket(obj.id);
  if (!bucket.ok()) {
    return bucket.status();
  }
  return bucket.value()->updateObject(obj);
}

absl::Status KVS::deleteObject(const geds::ObjectID &id) {
  auto bucket = getBucket(id);
  if (!bucket.ok()) {
    return bucket.status();
  }
  return bucket.value()->deleteObject(id.key);
}

absl::Status KVS::deleteObjectPrefix(const geds::ObjectID &id) {
  auto bucket = getBucket(id);
  if (!bucket.ok()) {
    return bucket.status();
  }
  return bucket.value()->deleteObjectPrefix(id.key);
}

absl::StatusOr<geds::Object> KVS::lookup(const geds::ObjectID &id) {
  auto bucket = getBucket(id);
  if (!bucket.ok()) {
    return bucket.status();
  }
  return bucket.value()->lookup(id.key);
}

absl::StatusOr<std::pair<std::vector<geds::Object>, std::vector<std::string>>>
KVS::listObjects(const geds::ObjectID &id, char delimiter) {
  auto bucket = getBucket(id);
  if (!bucket.ok()) {
    return bucket.status();
  }
  return bucket.value()->listObjects(id.key, delimiter);
}
