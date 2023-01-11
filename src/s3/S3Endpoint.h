/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#ifndef GEDS_S3_ENDPOINT_H
#define GEDS_S3_ENDPOINT_H

#include <aws/s3/S3Errors.h>
#include <iostream>
#include <memory>
#include <optional>
#include <set>

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/s3/S3Client.h>

#include "GEDSFileStatus.h"
#include "Statistics.h"

namespace geds::s3 {

class Endpoint {
  mutable std::shared_ptr<geds::StatisticsCounter> numBytesReadCount =
      geds::Statistics::counter("S3Endpoint: bytes read");
  mutable std::shared_ptr<geds::StatisticsCounter> totalRequestsSent =
      geds::Statistics::counter("S3Endpoint: requests sent");

public:
  const std::string _endpointUrl;
  const std::string _accessKey;
  std::shared_ptr<Aws::S3::S3Client> _s3Client;
  Aws::Auth::AWSCredentials _s3Credentials;
  std::shared_ptr<Aws::Client::ClientConfiguration> _s3Config;

  absl::Status convertS3Error(const Aws::S3::S3Error &error, const std::string &action,
                              const std::string &key) const;

public:
  Endpoint() = delete;
  Endpoint(std::string endpointUrl, std::string accessKey, std::string secretKey);
  Endpoint &operator=(const Endpoint &) = delete;

  ~Endpoint();

  /**
   * @brief List keys starting with prefix.
   */
  absl::StatusOr<std::vector<GEDSFileStatus>> list(const std::string &bucket,
                                                   const std::string &prefix) const;

  /**
   * @brief List keys starting with prefix. A non-zero delimiter will compute common prefixes and
   * emulates folders. Keys ending with `_$folder$` will be used as directory markers.
   */
  absl::StatusOr<std::vector<GEDSFileStatus>>
  list(const std::string &bucket, const std::string &prefix, char delimiter,
       std::optional<std::string> continuationToken = std::nullopt) const;

  absl::Status list(const std::string &bucket, const std::string &prefix, char delimiter,
                    std::set<GEDSFileStatus> &result,
                    std::optional<std::string> continuationToken = std::nullopt) const;
  /**
   * @brief List keys starting with prefix. `/` will be used as delmiter and acts as folder
   * emulation. Keys ending with `/_$folder$` will be used as directory markers.
   */
  absl::StatusOr<std::vector<GEDSFileStatus>> listAsFolder(const std::string &bucket,
                                                           const std::string &prefix) const;

  absl::StatusOr<GEDSFileStatus> fileStatus(const std::string &bucket,
                                            const std::string &key) const;
  absl::StatusOr<GEDSFileStatus> folderStatus(const std::string &bucket, const std::string &key,
                                              char delimiter) const;

  absl::Status deleteObject(const std::string &bucket, const std::string &key);
  absl::Status deletePrefix(const std::string &bucket, const std::string &prefix);

  absl::Status putObject(const std::string &bucket, const std::string &key, const uint8_t *bytes,
                         size_t length);

  absl::Status putObject(const std::string &bucket, const std::string &key,
                         std::shared_ptr<std::iostream> stream);

  absl::StatusOr<size_t> readBytes(const std::string &bucket, const std::string &key,
                                   uint8_t *bytes, size_t position, size_t length) const;

  absl::StatusOr<size_t> read(const std::string &bucket, const std::string &key,
                              std::iostream &outputStream,
                              std::optional<size_t> position = std::nullopt,
                              std::optional<size_t> length = std::nullopt,
                              bool retry = false) const;
};

} // namespace geds::s3

#endif
