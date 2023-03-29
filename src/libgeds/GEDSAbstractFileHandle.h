/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>

#include "Filesystem.h"
#include "GEDSFile.h"
#include "GEDSFileHandle.h"
#include "Logging.h"
#include "MMAPFile.h"
#include "Statistics.h"

namespace geds::service {
std::string getLocalPath(std::shared_ptr<GEDS> geds, const std::string &bucket,
                         const std::string &key);
absl::Status seal(std::shared_ptr<GEDS> geds, GEDSFileHandle &fileHandle, bool update, size_t size);
} // namespace geds::service

template <class T> class GEDSAbstractFileHandle : public GEDSFileHandle {
  bool _isSealed{false};
  size_t _sealedSize{0};

  bool _isValid{true};

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
  ~GEDSAbstractFileHandle() override {
    if (_isValid) {
      // TODO: Should we delete the file.
      LOG_INFO("The file associated with", identifier, " (", _file.path(),
               ") is still registered.");
    }
  }

  absl::StatusOr<size_t> size() const override { return _file.size(); }
  virtual size_t sealedSize() const { return _sealedSize; }
  bool isWriteable() const override { return true; }

  absl::Status setMetadata(std::optional<std::string> metadata, bool seal) override {
    _metadata = std::move(metadata);
    if (seal) {
      return this->seal();
    }
    return absl::OkStatus();
  }

  absl::StatusOr<size_t> readBytes(uint8_t *bytes, size_t position, size_t length) override {
    auto result = _file.readBytes(bytes, position, length);
    if (result.ok()) {
      *_readStatistics += *result;
    }
    return result;
  }

  absl::Status writeBytes(const uint8_t *bytes, size_t position, size_t length) override {
    auto result = _file.writeBytes(bytes, position, length);
    if (result.ok()) {
      *_writeStatistics += length;
    }
    return result;
  }

  absl::Status write(std::istream &stream, size_t position,
                     std::optional<size_t> lengthOptional) override {
    auto result = _file.write(stream, position, lengthOptional);
    if (result.ok()) {
      *_writeStatistics += *result;
    }
    return absl::OkStatus();
  }

  absl::Status truncate(size_t targetSize) override { return _file.truncate(targetSize); }

  absl::Status seal() override {
    auto lock = lockFile();
    size_t currentSize = _file.size();
    absl::Status status = absl::OkStatus();
    // FIXME: Create a GEDS Service mock to skip this abonimation below here.
    if (_gedsService != nullptr) { // Allow faking the GEDS Service for unittests.
      status = geds::service::seal(_gedsService, *this, _isSealed, currentSize);
    }
    if (status.ok()) {
      _isSealed = true;
      _sealedSize = currentSize;
    }
    return status;
  }

  void notifyUnused() override {
    auto lock = lockFile();
    if (_openCount > 0) {
      return;
    }
    _file.notifyUnused();
  };

  bool isValid() const override { return _isValid; }

  absl::StatusOr<int> rawFd() const override { return _file.rawFd(); }
};
