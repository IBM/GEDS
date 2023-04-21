/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>

#include "GEDSFileHandle.h"

/**
 * @brief A file handle that supports relocation of objects.
 */
class GEDSRelocatableFileHandle : public GEDSFileHandle {

  std::atomic<bool> _retrying{false};
  std::shared_ptr<GEDSFileHandle> _fileHandle;
  mutable std::shared_mutex _fileHandleMutex;
  mutable std::mutex _retryMutex;

private:
  GEDSRelocatableFileHandle(std::shared_ptr<GEDS> gedsService,
                            std::shared_ptr<GEDSFileHandle> fileHandle)
      : GEDSFileHandle(gedsService, fileHandle->bucket, fileHandle->key, std::nullopt),
        _fileHandle(fileHandle) {}

public:
  GEDSRelocatableFileHandle() = delete;
  GEDSRelocatableFileHandle(GEDSRelocatableFileHandle &) = delete;
  GEDSRelocatableFileHandle(GEDSRelocatableFileHandle &&) = delete;
  GEDSRelocatableFileHandle &operator=(GEDSRelocatableFileHandle &) = delete;
  GEDSRelocatableFileHandle &operator=(GEDSRelocatableFileHandle &&) = delete;
  ~GEDSRelocatableFileHandle() override = default;

  [[nodiscard]] static std::shared_ptr<GEDSFileHandle>
  factory(std::shared_ptr<GEDS> gedsService, std::shared_ptr<GEDSFileHandle> wrapped);

  bool isRelocatable() const override;

  absl::StatusOr<size_t> size() const override;

  size_t localStorageSize() const override;

  size_t localMemorySize() const override;

  bool isWriteable() const override;

  std::optional<std::string> metadata() const override;

  absl::Status setMetadata(std::optional<std::string> metadata, bool seal) override;

  absl::StatusOr<size_t> readBytes(uint8_t *bytes, size_t position, size_t length) override;

  absl::Status writeBytes(const uint8_t *bytes, size_t position, size_t length) override;

  absl::Status write(std::istream &stream, size_t position,
                     std::optional<size_t> lengthOptional) override;

  absl::Status truncate(size_t targetSize) override;

  absl::Status seal() override;

  void notifyUnused() override;

  absl::StatusOr<int> rawFd() const override;

  absl::StatusOr<std::shared_ptr<GEDSFileHandle>> relocate() override;
};
