/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <optional>
#include <unistd.h>
#include <vector>

#include "Filesystem.h"
#include "GEDSFile.h"
#include "GEDSFileHandle.h"
#include "GEDSS3FileHandle.h"
#include "Logging.h"
#include "MMAPFile.h"
#include "Statistics.h"

namespace geds::service {
std::string getLocalPath(std::shared_ptr<GEDS> geds, const std::string &bucket,
                         const std::string &key);
absl::Status seal(std::shared_ptr<GEDS> geds, GEDSFileHandle &fileHandle, bool update, size_t size);
absl::StatusOr<std::shared_ptr<geds::s3::Endpoint>> getS3Endpoint(std::shared_ptr<GEDS> geds,
                                                                  const std::string &bucket);

} // namespace geds::service

template <class T> class GEDSAbstractFileHandle : public GEDSFileHandle {
  bool _isSealed{false};

  T _file;

  std::shared_ptr<geds::StatisticsCounter> _readStatistics;
  std::shared_ptr<geds::StatisticsCounter> _writeStatistics;

private:
  // Constructors are private to enable `shared_from_this`.
  GEDSAbstractFileHandle(std::shared_ptr<GEDS> gedsService, std::string bucketArg,
                         std::string keyArg, std::optional<std::string> metadataArg,
                         std::string pathArg)
      : GEDSFileHandle(gedsService, std::move(bucketArg), std::move(keyArg),
                       std::move(metadataArg)),
        _file(T(std::move(pathArg))), _readStatistics(geds::Statistics::createCounter(
                                          "GEDS" + _file.statisticsLabel() + "Handle: bytes read")),
        _writeStatistics(geds::Statistics::createCounter("GEDS" + _file.statisticsLabel() +
                                                         "Handle: bytes written")) {
    static auto counter =
        geds::Statistics::createCounter("GEDS" + _file.statisticsLabel() + "Handle: count");
    *counter += 1;
  }

public:
  [[nodiscard]] static absl::StatusOr<std::shared_ptr<GEDSFileHandle>>
  factory(std::shared_ptr<GEDS> gedsService, std::string bucketArg, std::string keyArg,
          std::optional<std::string> metadataArg,
          std::optional<std::string> pathArg = std::nullopt) {
    try {
      auto path = pathArg.has_value() ? pathArg.value()
                                      : geds::service::getLocalPath(gedsService, bucketArg, keyArg);
      auto pathDir = std::filesystem::path(path).remove_filename();
      auto dirStatus = geds::filesystem::mkdir(pathDir);
      if (!dirStatus.ok()) {
        return dirStatus;
      }
      return std::shared_ptr<GEDSFileHandle>(
          new GEDSAbstractFileHandle<T>(std::move(gedsService), std::move(bucketArg),
                                        std::move(keyArg), std::move(metadataArg), path));
    } catch (const std::runtime_error &e) {
      return absl::UnknownError(e.what());
    }
  }

  GEDSAbstractFileHandle() = delete;
  GEDSAbstractFileHandle(GEDSAbstractFileHandle &) = delete;
  GEDSAbstractFileHandle(GEDSAbstractFileHandle &&) = delete;
  GEDSAbstractFileHandle &operator=(GEDSAbstractFileHandle &) = delete;
  GEDSAbstractFileHandle &operator=(GEDSAbstractFileHandle &&) = delete;
  ~GEDSAbstractFileHandle() override = default;

  bool isRelocatable() const override { return true; }
  absl::StatusOr<size_t> size() const override { return _file.size(); }
  size_t localStorageSize() const override { return _file.localStorageSize(); }
  size_t localMemorySize() const override { return _file.localMemorySize(); }

  bool isWriteable() const override { return true; }

  absl::Status setMetadata(std::optional<std::string> metadata, bool seal) override {
    auto lock = lockFile();
    _metadata = std::move(metadata);
    if (seal) {
      return this->seal();
    }
    return absl::OkStatus();
  }

  absl::StatusOr<size_t> readBytes(uint8_t *bytes, size_t position, size_t length) override {
    auto lock = lockShared();
    auto result = _file.readBytes(bytes, position, length);
    if (result.ok()) {
      *_readStatistics += *result;
    }
    return result;
  }

  absl::Status writeBytes(const uint8_t *bytes, size_t position, size_t length) override {
    auto lock = lockShared();
    auto result = _file.writeBytes(bytes, position, length);
    if (result.ok()) {
      *_writeStatistics += length;
    }
    return result;
  }

  absl::Status write(std::istream &stream, size_t position,
                     std::optional<size_t> lengthOptional) override {
    auto lock = lockShared();
    auto result = _file.write(stream, position, lengthOptional);
    if (result.ok()) {
      *_writeStatistics += *result;
    }
    return absl::OkStatus();
  }

  absl::Status truncate(size_t targetSize) override {
    auto lock = lockExclusive();
    return _file.truncate(targetSize);
  }

  absl::Status seal() override {
    auto ioLock = lockExclusive();
    auto lock = lockFile();
    size_t currentSize = _file.size();
    absl::Status status = absl::OkStatus();
    // FIXME: Create a GEDS Service mock to skip this abonimation below here.
    if (_gedsService != nullptr) { // Allow faking the GEDS Service for unittests.
      status = geds::service::seal(_gedsService, *this, _isSealed, currentSize);
    }
    if (status.ok()) {
      _isSealed = true;
    }
    return status;
  }

  void notifyUnused() override {
    auto iolock = lockExclusive();
    auto lock = lockFile();
    if (_openCount > 0) {
      return;
    }
    _file.notifyUnused();
  };

  absl::StatusOr<int> rawFd() const override {
    auto lock = lockShared();
    return _file.rawFd();
  }

  absl::StatusOr<std::shared_ptr<GEDSFileHandle>> relocate() override {
    auto iolock = lockExclusive();
    auto lock = lockFile();
    if (!isValid()) {
      return absl::UnavailableError("The file " + identifier + " is no longer valid!");
    }
    LOG_INFO("Relocating ", identifier);
    if (_openCount > 0) {
      auto message = "Unable to relocate " + identifier + " reason: The file is still in use.";
      LOG_ERROR(message);
      return absl::UnavailableError(message);
    }
    auto s3Endpoint = geds::service::getS3Endpoint(_gedsService, bucket);
    if (!s3Endpoint.ok()) {
      auto message =
          "Unable to relocate " + identifier + " reason: No tier configured for " + bucket;
      LOG_ERROR(message);
      return absl::UnavailableError(message);
    }

    absl::Status s3Put;
    auto rawPtr = _file.rawPtr();
    if (rawPtr.ok()) {
      s3Put = (*s3Endpoint)->putObject(bucket, key, *rawPtr, _file.size());
    } else {
      auto stream =
          std::make_shared<std::fstream>(_file.path(), std::ios_base::binary | std::ios_base::in);
      s3Put = (*s3Endpoint)->putObject(bucket, key, stream, std::make_optional(_file.size()));
    }
    if (!s3Put.ok()) {
      auto message =
          "Unable to relocate " + identifier + " to s3: Reason " + std::string{s3Put.message()};
      LOG_ERROR(message);
      return absl::UnknownError(message);
    }
    auto fh = GEDSS3FileHandle::factory(_gedsService, bucket, key, metadata());
    if (!fh.ok()) {
      LOG_ERROR("Unable to reopen the relocateed file ", identifier,
                " on s3:", fh.status().message());
      return fh.status();
    }
    auto status = (*fh)->seal();
    if (!status.ok()) {
      LOG_ERROR("Unable to seal relocateed file!");
      (void)(*s3Endpoint)->deleteObject(bucket, key);
      return status;
    }
    // Mark as invalid.
    _isValid = false;
    return fh;
  }
};
