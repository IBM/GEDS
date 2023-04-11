/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <absl/status/status.h>
#include <absl/status/statusor.h>

#include "MDSKVSBucket.h"
#include "Object.h"
#include "RWConcurrentObjectAdaptor.h"

class MDSKVS : public utility::RWConcurrentObjectAdaptor {
private:
  std::map<std::string, std::shared_ptr<MDSKVSBucket>> _map;

  absl::StatusOr<std::shared_ptr<MDSKVSBucket>> getBucket(const std::string &bucket);
  absl::StatusOr<std::shared_ptr<MDSKVSBucket>> getBucket(const geds::ObjectID &id);
  bool isCachePopulated;

public:
  MDSKVS();

  ~MDSKVS() = default;

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
  absl::Status bucketStatus(const std::string_view &bucket);

  /**
   * @brief Create object.
   */
  absl::Status createObject(const geds::Object &obj, bool forceCreateBucket = false);

  /**
   * @brief Update object.
   */
  absl::Status updateObject(const geds::Object &obj);

  /**
   * @brief Delete object with `id`.
   */
  absl::Status deleteObject(const geds::ObjectID &id);
  absl::Status deleteObject(const std::string &bucket, const std::string &key);

  /**
   * @brief Delete object in `id.bucket` with key starting with `id.key`.
   */
  absl::Status deleteObjectPrefix(const geds::ObjectID &id);
  absl::Status deleteObjectPrefix(const std::string &bucket, const std::string &prefix);

  /**
   * @brief Lookup exact object.
   */
  absl::StatusOr<geds::Object> lookup(const geds::ObjectID &id);
  absl::StatusOr<geds::Object> lookup(const std::string &bucket, const std::string &key);

  /**
   * @brief List objects and common prefixes starting with `id.key` as prefix and do not contain
   * `delimiter` in the postfix.
   * '\0' skips filtering by delimiter.
   */
  absl::StatusOr<std::pair<std::vector<geds::Object>, std::vector<std::string>>>
  listObjects(const geds::ObjectID &id, char delimiter = 0);

  absl::StatusOr<std::pair<std::vector<geds::Object>, std::vector<std::string>>>
  listObjects(const std::string &bucket, const std::string &prefix, char delimiter = 0);
};
