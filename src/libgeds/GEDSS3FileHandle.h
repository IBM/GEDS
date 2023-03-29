/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GEDS_S3_FILE_HANDLE_H
#define GEDS_S3_FILE_HANDLE_H

#include "GEDSFileHandle.h"
#include "Object.h"
#include "S3Endpoint.h"
#include "Statistics.h"
#include <optional>

class GEDSS3FileHandle : public GEDSFileHandle {
public:
  const std::string s3Bucket;
  const std::string s3Key;
  const std::string location;

protected:
  const std::shared_ptr<geds::s3::Endpoint> _s3Endpoint;
  size_t _size;
  bool _isValid{true};
  std::shared_ptr<geds::StatisticsCounter> _readStatistics =
      geds::Statistics::createCounter("GEDSS3FileHandle: bytes read");

private:
  GEDSS3FileHandle(std::shared_ptr<GEDS> gedsService,
                   std::shared_ptr<geds::s3::Endpoint> objectStore, const std::string &bucket,
                   const std::string &key, std::optional<std::string> metadataArg,
                   const std::string &s3Bucket, const std::string &s3Key,
                   std::optional<size_t> fileSize = std::nullopt);

public:
  [[nodiscard]] static absl::StatusOr<std::shared_ptr<GEDSFileHandle>>
  factory(std::shared_ptr<GEDS> gedsService, const geds::Object &object);

  [[nodiscard]] static absl::StatusOr<std::shared_ptr<GEDSFileHandle>>
  factory(std::shared_ptr<GEDS> gedsService, const std::string &bucket, const std::string &key,
          std::optional<std::string> metadataArg);

  bool isValid() const override;
  absl::StatusOr<size_t> size() const override;

  absl::StatusOr<size_t> readBytes(uint8_t *bytes, size_t position, size_t length) override;

  absl::StatusOr<size_t> downloadRange(std::shared_ptr<GEDSFileHandle> destination,
                                       size_t srcPosition, size_t length,
                                       size_t destPosition) override;

  absl::Status seal() override;
};

#endif
