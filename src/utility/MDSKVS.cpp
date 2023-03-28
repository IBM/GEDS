/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "MDSKVS.h"
#include "absl/status/status.h"

#define QUOTE(str) #str
#define STRINGIFY(str) QUOTE(str)

MDSKVS::MDSKVS() {
#if defined(HAVE_DEFAULT_BUCKET) && HAVE_DEFAULT_BUCKET
  (void)createBucket(STRINGIFY(DEFAULT_BUCKET_NAME));
#endif
}

MDSKVS::~MDSKVS() {}

absl::StatusOr<std::shared_ptr<MDSKVSBucket>> MDSKVS::getBucket(const std::string &bucket) {
  auto lock = getReadLock();
  auto it = _map.find(bucket);
  if (it == _map.end()) {
    return absl::NotFoundError("Bucket " + bucket + " does not exist.");
  }
  return it->second;
}

absl::StatusOr<std::shared_ptr<MDSKVSBucket>> MDSKVS::getBucket(const geds::ObjectID &id) {
  return getBucket(id.bucket);
}

absl::Status MDSKVS::createBucket(const std::string &bucket) {
  auto lock = getWriteLock();
  if (_map.find(bucket) != _map.end()) {
    return absl::AlreadyExistsError("Bucket " + bucket + " already exists.");
  }
  _map[bucket] = std::make_shared<MDSKVSBucket>(bucket);
  return absl::OkStatus();
}

absl::Status MDSKVS::deleteBucket(const std::string &bucket) {
  auto lock = getWriteLock();
  auto it = _map.find(bucket);
  if (it == _map.end()) {
    return absl::NotFoundError("Bucket " + bucket + " does not exist.");
  }
  _map.erase(it);
  return absl::OkStatus();
}

absl::StatusOr<std::vector<std::string>> MDSKVS::listBuckets() {
  auto lock = getReadLock();
  std::vector<std::string> result;
  result.reserve(_map.size());
  for (const auto &item : _map) {
    result.push_back(item.first);
  }
  return result;
}

absl::Status MDSKVS::bucketStatus(const std::string &bucket) {
  auto lock = getReadLock();
  if (_map.find(bucket) != _map.end()) {
    return absl::OkStatus();
  }
  return absl::NotFoundError("Bucket " + bucket + " does not exist.");
}

absl::Status MDSKVS::createObject(const geds::Object &obj, bool forceCreateBucket) {
  auto bucket = getBucket(obj.id);
  if (!bucket.ok()) {
    if (!forceCreateBucket) {
      return bucket.status();
    }
    auto s = createBucket(obj.id.bucket);
    if (!s.ok()) {
      return s;
    }
    bucket = getBucket(obj.id);
    if (!bucket.ok()) {
      return bucket.status();
    }
  }
  return (*bucket)->createObject(obj);
}

absl::Status MDSKVS::updateObject(const geds::Object &obj) {
  auto bucket = getBucket(obj.id);
  if (!bucket.ok()) {
    return bucket.status();
  }
  return bucket.value()->updateObject(obj);
}

absl::Status MDSKVS::deleteObject(const geds::ObjectID &id) {
  auto bucket = getBucket(id);
  if (!bucket.ok()) {
    return bucket.status();
  }

  return bucket.value()->deleteObject(id.key);
}
absl::Status MDSKVS::deleteObject(const std::string &bucket, const std::string &key) {
  auto b = getBucket(bucket);
  if (!b.ok()) {
    return b.status();
  }
  return (*b)->deleteObject(key);
}

absl::Status MDSKVS::deleteObjectPrefix(const geds::ObjectID &id) {
  auto bucket = getBucket(id);
  if (!bucket.ok()) {
    return bucket.status();
  }
  return bucket.value()->deleteObjectPrefix(id.key);
}

absl::Status MDSKVS::deleteObjectPrefix(const std::string &bucket, const std::string &prefix) {
  auto b = getBucket(bucket);
  if (!b.ok()) {
    return b.status();
  }
  return (*b)->deleteObjectPrefix(prefix);
}

absl::StatusOr<geds::Object> MDSKVS::lookup(const geds::ObjectID &id) {
  auto bucket = getBucket(id);
  if (!bucket.ok()) {
    return bucket.status();
  }
  return bucket.value()->lookup(id.key);
}

absl::StatusOr<geds::Object> MDSKVS::lookup(const std::string &bucket, const std::string &key) {
  auto b = getBucket(bucket);
  if (!b.ok()) {
    return b.status();
  }
  return (*b)->lookup(key);
}

absl::StatusOr<std::pair<std::vector<geds::Object>, std::vector<std::string>>>
MDSKVS::listObjects(const geds::ObjectID &id, char delimiter) {
  auto bucket = getBucket(id);
  if (!bucket.ok()) {
    return bucket.status();
  }
  return bucket.value()->listObjects(id.key, delimiter);
}

absl::StatusOr<std::pair<std::vector<geds::Object>, std::vector<std::string>>>
MDSKVS::listObjects(const std::string &bucket, const std::string &prefix, char delimiter) {
  auto b = getBucket(bucket);
  if (!b.ok()) {
    return b.status();
  }
  return (*b)->listObjects(prefix, delimiter);
}
