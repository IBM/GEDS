/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cassert>
#include <ios>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

#include "GEDS.h"
#include "GEDSFileHandle.h"
#include "GEDSS3FileHandle.h"
#include "Logging.h"

GEDSS3FileHandle::GEDSS3FileHandle(std::shared_ptr<GEDS> gedsService,
                                   std::shared_ptr<geds::s3::Endpoint> s3Endpoint,
                                   const std::string &bucketArg, const std::string &keyArg,
                                   const std::string &s3BucketArg, const std::string &s3KeyArg,
                                   std::optional<size_t> fileSize)
    : GEDSFileHandle(gedsService, bucketArg, keyArg), s3Bucket(s3BucketArg), s3Key(s3KeyArg),
      location("s3://" + s3Bucket + "/" + s3Key), _s3Endpoint(s3Endpoint) {
  static auto counter = geds::Statistics::createCounter("GEDSS3FileHandle: count");
  *counter += 1;
  if (fileSize.has_value()) {
    _size = *fileSize;
  } else {
    auto sizeStatus = _s3Endpoint->fileStatus(s3Bucket, s3Key);
    if (!sizeStatus.ok()) {
      throw sizeStatus.status();
    }
    _size = sizeStatus->size;
  }
}

absl::StatusOr<std::shared_ptr<GEDSFileHandle>>
GEDSS3FileHandle::factory(std::shared_ptr<GEDS> gedsService, const geds::Object &object) {
  const auto &bucket = object.id.bucket;
  const auto &key = object.id.key;
  const auto &location = object.info.location;
  const std::string_view prefix{"s3://"};

  if (location.compare(0, prefix.size(), prefix) != 0) {
    return absl::UnknownError("Object location has invalid prefix for S3 FileHandle: " + location +
                              " expected '" + std::string{prefix} + "' as prefix.");
  }
  auto splitpos = location.find('/', prefix.size());
  if (splitpos == std::string::npos) {
    return absl::UnknownError(location + " invalid! Expected format: " + std::string{prefix} +
                              "bucket/path");
  }
  auto s3Bucket = location.substr(prefix.size(), splitpos - prefix.size());
  auto s3Key = location.substr(splitpos + 1);

  auto s3Endpoint = gedsService->getS3Endpoint(s3Bucket);
  if (!s3Endpoint.ok()) {
    return s3Endpoint.status();
  }
  auto exists = (*s3Endpoint)->fileStatus(s3Bucket, s3Key);
  if (!exists.ok()) {
    return exists.status();
  }
  try {
    return std::shared_ptr<GEDSFileHandle>(
        new GEDSS3FileHandle(gedsService, s3Endpoint.value(), bucket, key, s3Bucket, s3Key));
  } catch (absl::Status &err) {
    return err;
  }
}

absl::StatusOr<std::shared_ptr<GEDSFileHandle>>
GEDSS3FileHandle::factory(std::shared_ptr<GEDS> gedsService, const std::string &bucket,
                          const std::string &key) {
  auto s3Endpoint = gedsService->getS3Endpoint(bucket);
  if (!s3Endpoint.ok()) {
    return s3Endpoint.status();
  }
  auto exists = (*s3Endpoint)->fileStatus(bucket, key);
  if (!exists.ok()) {
    return exists.status();
  }
  try {
    return std::shared_ptr<GEDSS3FileHandle>(
        new GEDSS3FileHandle(gedsService, s3Endpoint.value(), bucket, key, bucket, key));
  } catch (absl::Status &err) {
    return err;
  }
}

bool GEDSS3FileHandle::isValid() const { return _isValid; }

absl::StatusOr<size_t> GEDSS3FileHandle::size() const { return _size; }

absl::StatusOr<size_t> GEDSS3FileHandle::readBytes(uint8_t *bytes, size_t position, size_t length) {
  if (!_isValid) {
    return absl::NotFoundError("The file is no longer valid!");
  }
  auto status = _s3Endpoint->readBytes(s3Bucket, s3Key, bytes, position, length);
  if (!status.ok() || status.status().code() == absl::StatusCode::kNotFound) {
    _isValid = false;
  }
  *_readStatistics += status.value_or(0);
  return status;
}

absl::StatusOr<size_t> GEDSS3FileHandle::downloadRange(std::shared_ptr<GEDSFileHandle> destination,
                                                       size_t srcPosition, size_t length,
                                                       size_t destPosition) {
  std::stringstream outputStream;
  auto count = _s3Endpoint->read(bucket, key, outputStream, srcPosition, length);
  if (!count.ok()) {
    return count.status();
  }
  *_readStatistics += *count;
  auto writeStatus = destination->write(outputStream, destPosition, *count);
  if (!writeStatus.ok()) {
    return writeStatus;
  }
  return *count;
}

absl::Status GEDSS3FileHandle::seal() { return _gedsService->seal(*this, false, _size, location); }
