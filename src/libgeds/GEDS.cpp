/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "GEDS.h"
#include "absl/status/status.h"

std::shared_ptr<GEDS> GEDS::factory() { return std::shared_ptr<GEDS>(new GEDS()); }

GEDS::~GEDS() {
  // TODO: NYI
}

absl::Status GEDS::start() { return absl::UnimplementedError("NYI"); }

absl::Status GEDS::stop() { return absl::UnimplementedError("NYI"); }

absl::StatusOr<GEDSFile> GEDS::create(const std::string &objectName, bool overwrite) {
  return absl::UnimplementedError("NYI");
}
absl::StatusOr<GEDSFile> GEDS::create(const std::string &bucket, const std::string &key,
                                      bool overwrite) {
  return absl::UnimplementedError("NYI");
}

absl::StatusOr<std::shared_ptr<GEDSFileHandle>> GEDS::createAsFileHandle(const std::string &bucket,
                                                                         const std::string &key,
                                                                         bool overwrite = false) {
  return absl::UnimplementedError("NYI");
}

absl::Status GEDS::mkdirs(const std::string &bucket, const std::string &path, char delimiter) {
  return absl::UnimplementedError("NYI");
}

absl::Status GEDS::createBucket(const std::string &bucket) {
  return absl::UnimplementedError("NYI");
}

absl::Status GEDS::lookupBucket(const std::string &bucket) {
  return absl::UnimplementedError("NYI");
}

absl::StatusOr<GEDSFile> GEDS::open(const std::string &objectName) {
  return absl::UnimplementedError("NYI");
}
absl::StatusOr<GEDSFile> GEDS::open(const std::string &bucket, const std::string &key, bool retry) {
  return absl::UnimplementedError("NYI");
}
absl::StatusOr<std::shared_ptr<GEDSFileHandle>> GEDS::openAsFileHandle(const std::string &bucket,
                                                                       const std::string &key) {
  return absl::UnimplementedError("NYI");
}
absl::StatusOr<std::shared_ptr<GEDSFileHandle>>
GEDS::reopenFileHandle(const std::string &bucket, const std::string &key, bool invalidate) {
  return absl::UnimplementedError("NYI");
}

absl::StatusOr<std::shared_ptr<GEDSFileHandle>>
GEDS::reopen(std::shared_ptr<GEDSFileHandle> existing) {
  return absl::UnimplementedError("NYI");
}

absl::StatusOr<GEDSFile> GEDS::localOpen(const std::string &objectName) {
  return absl::UnimplementedError("NYI");
}
absl::StatusOr<GEDSFile> GEDS::localOpen(const std::string &bucket, const std::string &key) {
  return absl::UnimplementedError("NYI");
}

absl::Status GEDS::seal(GEDSFileHandle &fileHandle, bool update, size_t size,
                        std::optional<std::string> uri) {
  return absl::UnimplementedError("NYI");
}

absl::StatusOr<std::vector<GEDSFileStatus>> GEDS::list(const std::string &bucket,
                                                       const std::string &prefix) {
  return absl::UnimplementedError("NYI");
}

absl::StatusOr<std::vector<GEDSFileStatus>> list(const std::string &bucket,
                                                 const std::string &prefix, char delimiter) {
  return absl::UnimplementedError("NYI");
}

absl::StatusOr<std::vector<GEDSFileStatus>> GEDS::listAsFolder(const std::string &bucket,
                                                               const std::string &prefix) {
  return absl::UnimplementedError("NYI");
}

absl::StatusOr<GEDSFileStatus> GEDS::status(const std::string &bucket, const std::string &key);
absl::StatusOr<GEDSFileStatus> GEDS::status(const std::string &bucket, const std::string &key,
                                            char delimiter) {
  return absl::UnimplementedError("NYI");
}

absl::Status GEDS::renamePrefix(const std::string &bucket, const std::string &srcPrefix,
                                const std::string &destPrefix) {
  return absl::UnimplementedError("NYI");
}
absl::Status GEDS::renamePrefix(const std::string &srcBucket, const std::string &srcPrefix,
                                const std::string &destPrefix, const std::string &destKey) {
  return absl::UnimplementedError("NYI");
}

absl::Status GEDS::rename(const std::string &bucket, const std::string &srcKey,
                          const std::string &destKey) {
  return absl::UnimplementedError("NYI");
}
absl::Status GEDS::rename(const std::string &srcBucket, const std::string &srcKey,
                          const std::string &destBucket, const std::string &destKey) {
  return absl::UnimplementedError("NYI");
}

absl::Status GEDS::copyPrefix(const std::string &bucket, const std::string &srcPrefix,
                              const std::string &destPrefix) {
  return absl::UnimplementedError("NYI");
}
absl::Status copyPrefix(const std::string &srcBucket, const std::string &srcPrefix,
                        const std::string &destPrefix, const std::string &destKey) {
  return absl::UnimplementedError("NYI");
}

/**
 * @brief Copy an object.
 */
absl::Status copy(const std::string &bucket, const std::string &srcKey,
                  const std::string &destKey) {
  return absl::UnimplementedError("NYI");
}
absl::Status copy(const std::string &srcBucket, const std::string &srcKey,
                  const std::string &destBucket, const std::string &destKey) {
  return absl::UnimplementedError("NYI");
}

absl::Status deleteObject(const std::string &bucket, const std::string &key) {
  return absl::UnimplementedError("NYI");
}

absl::Status deleteObjectPrefix(const std::string &bucket, const std::string &prefix) {
  return absl::UnimplementedError("NYI");
}

absl::Status registerObjectStoreConfig(const std::string &bucket, const std::string &endpointUrl,
                                       const std::string &accessKey, const std::string &secretKey) {
  return absl::UnimplementedError("NYI");
}
