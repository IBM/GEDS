/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GEDS_GEDS_H
#define GEDS_GEDS_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <shared_mutex>
#include <string>
#include <utility>

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <boost/asio/thread_pool.hpp>

#include "GEDSFileHandle.h"
#include "GEDSFileStatus.h"
#include "RWConcurrentObjectAdaptor.h"

const char Default_GEDSFolderDelimiter = '/';

class GEDSFile;

class GEDS : public std::enable_shared_from_this<GEDS>, utility::RWConcurrentObjectAdaptor {

protected:
  GEDS() = default;

public:
  const std::string uuid;

  /**
   * @brief Constructor wrapper which forces a shared_ptr.
   */
  [[nodiscard]] static std::shared_ptr<GEDS> factory();

  virtual ~GEDS();

  /**
   * @brief Start GEDS.
   */
  absl::Status start();

  /**
   * @brief Stop GEDS.
   */
  absl::Status stop();

  /**
   * @brief Create object located at bucket/key.
   * The object is registered with the metadata service once the file is sealed.
   */
  absl::StatusOr<GEDSFile> create(const std::string &objectName, bool overwrite = false);
  absl::StatusOr<GEDSFile> create(const std::string &bucket, const std::string &key,
                                  bool overwrite = false);
  absl::StatusOr<std::shared_ptr<GEDSFileHandle>>
  createAsFileHandle(const std::string &bucket, const std::string &key, bool overwrite = false);

  /**
   * @brief Recursively create directory using directory markers.
   */
  absl::Status mkdirs(const std::string &bucket, const std::string &path,
                      char delimiter = Default_GEDSFolderDelimiter);

  /**
   * @brief Register a bucket with the metadata server.
   */
  absl::Status createBucket(const std::string &bucket);

  absl::Status lookupBucket(const std::string &bucket);

  /**
   * @brief Open object located at bucket/key.
   */
  absl::StatusOr<GEDSFile> open(const std::string &objectName);
  absl::StatusOr<GEDSFile> open(const std::string &bucket, const std::string &key,
                                bool retry = true);
  absl::StatusOr<std::shared_ptr<GEDSFileHandle>> openAsFileHandle(const std::string &bucket,
                                                                   const std::string &key);
  absl::StatusOr<std::shared_ptr<GEDSFileHandle>>
  reopenFileHandle(const std::string &bucket, const std::string &key, bool invalidate);

  /**
   * @brief Reopen a file after a unsuccessful read.
   */
  absl::StatusOr<std::shared_ptr<GEDSFileHandle>> reopen(std::shared_ptr<GEDSFileHandle> existing);

  /**
   * @brief Only open local filehandles.
   */
  absl::StatusOr<GEDSFile> localOpen(const std::string &objectName);
  absl::StatusOr<GEDSFile> localOpen(const std::string &bucket, const std::string &key);

  /**
   * @brief Mark the file associated with fileHandle as sealed.
   */
  absl::Status seal(GEDSFileHandle &fileHandle, bool update, size_t size,
                    std::optional<std::string> uri = std::nullopt);

  /**
   * @brief List objects in bucket where the key starts with `prefix`.
   */
  absl::StatusOr<std::vector<GEDSFileStatus>> list(const std::string &bucket,
                                                   const std::string &prefix);

  /**
   * @brief List objects in `bucket` where the key starts with `prefix` and the postfix does not
   * contain `delimiter`.
   * - If the delimiter set to `0` will list all keys starting with prefix.
   * - If the delimiter is set to a value != 0, then the delimiter will be used as a folder
   * separator. Keys ending with "/_$folder$" will be used as directory markers (where '/' is used
   * as a delimiter).
   */
  absl::StatusOr<std::vector<GEDSFileStatus>> list(const std::string &bucket,
                                                   const std::string &prefix, char delimiter);

  /**
   * @brief List objects in `bucket` with `/` acting as delimiter.
   */
  absl::StatusOr<std::vector<GEDSFileStatus>> listAsFolder(const std::string &bucket,
                                                           const std::string &prefix);

  /**
   * @brief Get status of `key` in `bucket`.
   */
  absl::StatusOr<GEDSFileStatus> status(const std::string &bucket, const std::string &key);
  absl::StatusOr<GEDSFileStatus> status(const std::string &bucket, const std::string &key,
                                        char delimiter);

  /**
   * @brief Rename a prefix recursively.
   */
  absl::Status renamePrefix(const std::string &bucket, const std::string &srcPrefix,
                            const std::string &destPrefix);
  absl::Status renamePrefix(const std::string &srcBucket, const std::string &srcPrefix,
                            const std::string &destPrefix, const std::string &destKey);

  /**
   * @brief Rename an object.
   */
  absl::Status rename(const std::string &bucket, const std::string &srcKey,
                      const std::string &destKey);
  absl::Status rename(const std::string &srcBucket, const std::string &srcKey,
                      const std::string &destBucket, const std::string &destKey);

  /**
   * @brief Copy a file or a folder structure.
   */
  absl::Status copyPrefix(const std::string &bucket, const std::string &srcPrefix,
                          const std::string &destPrefix);
  absl::Status copyPrefix(const std::string &srcBucket, const std::string &srcPrefix,
                          const std::string &destPrefix, const std::string &destKey);

  /**
   * @brief Copy an object.
   */
  absl::Status copy(const std::string &bucket, const std::string &srcKey,
                    const std::string &destKey);
  absl::Status copy(const std::string &srcBucket, const std::string &srcKey,
                    const std::string &destBucket, const std::string &destKey);

  /**
   * @brief Delete object in `bucket` with `key`.
   * @returns absl::OkStatus if the operation has been successful or not objects have been deleted.
   */
  absl::Status deleteObject(const std::string &bucket, const std::string &key);

  /**
   * @brief Delete objects in `bucket` with keys starting with `prefix`.
   * @returns absl::OkStatus if the operation has been successful or not objects have been deleted.
   */
  absl::Status deleteObjectPrefix(const std::string &bucket, const std::string &prefix);

  /**
   * @brief Register an object store configuration with GEDS.
   */
  absl::Status registerObjectStoreConfig(const std::string &bucket, const std::string &endpointUrl,
                                         const std::string &accessKey,
                                         const std::string &secretKey);
};

#endif // GEDS_GEDS_H
