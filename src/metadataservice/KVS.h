/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#ifndef GEDS_KVS_H
#define GEDS_KVS_H

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <absl/status/statusor.h>

#include "Common.h"
#include "KVSBucket.h"
#include "Object.h"

class KVS {
private:
  std::shared_mutex _mutex;

  std::shared_lock<std::shared_mutex> getReadLock() {
    return std::shared_lock<std::shared_mutex>(_mutex);
  }

  std::unique_lock<std::shared_mutex> getWriteLock() {
    return std::unique_lock<std::shared_mutex>(_mutex);
  }

  std::map<std::string, std::shared_ptr<KVSBucket>> _map;

  absl::StatusOr<std::shared_ptr<KVSBucket>> getBucket(const std::string &bucket);
  absl::StatusOr<std::shared_ptr<KVSBucket>> getBucket(const geds::ObjectID &id);

public:
  KVS();

  ~KVS();

  /**
   * @brief Create bucket.
   */
  absl::Status createBucket(const std::string &bucket);

  /**
   * @brief Delete bucket.
   */
  absl::Status deleteBucket(const std::string &bucket);

  /**
   * @brief List buckets.
   *
   * @return absl::StatusOr<std::vector<std::string>>
   */
  absl::StatusOr<std::vector<std::string>> listBuckets();

  /**
   * @brief Check bucket status.
   */
  absl::Status bucketStatus(const std::string &bucket);

  /**
   * @brief Create object.
   */
  absl::Status createObject(const geds::Object &obj);

  /**
   * @brief Update object.
   */
  absl::Status updateObject(const geds::Object &obj);

  /**
   * @brief Delete object with `id`.
   */
  absl::Status deleteObject(const geds::ObjectID &id);

  /**
   * @brief Delete object in `id.bucket` with key starting with `id.key`.
   */
  absl::Status deleteObjectPrefix(const geds::ObjectID &id);

  /**
   * @brief Lookup exact object.
   */
  absl::StatusOr<geds::Object> lookup(const geds::ObjectID &id);

  /**
   * @brief List objects and common prefixes starting with `id.key` as prefix and do not contain
   * `delimiter` in the postfix.
   * '\0' skips filtering by delimiter.
   */
  absl::StatusOr<std::pair<std::vector<geds::Object>, std::vector<std::string>>>
  listObjects(const geds::ObjectID &id, char delimiter = 0);
};

#endif // GEDS_KVS_H
