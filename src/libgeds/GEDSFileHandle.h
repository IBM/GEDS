/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>

#include <absl/status/status.h>
#include <absl/status/statusor.h>

#include "GEDSFile.h"
#include "GEDSInternal.h"

class GEDS;

class GEDSFileHandle : public std::enable_shared_from_this<GEDSFileHandle> {
public:
  const std::string bucket;
  const std::string key;
  const std::string identifier;

protected:
  std::atomic<int64_t> _openCount{0};

  /** Mutex for file-based operations. */
  mutable std::recursive_mutex _fileMutex;

  std::optional<std::string> _metadata = std::nullopt;
  bool _isValid{true};
  std::chrono::system_clock::time_point _lastOpened;
  std::chrono::system_clock::time_point _lastReleased;

  /** Mutex for IO operations.*/
  mutable std::shared_mutex _ioMutex;
  auto lockShared() const { return std::shared_lock<std::shared_mutex>(_ioMutex); }
  auto lockExclusive() const { return std::unique_lock<std::shared_mutex>(_ioMutex); }

  std::shared_ptr<GEDS> _gedsService;

  // Constructors are private to enable `shared_from_this`.
  GEDSFileHandle(std::shared_ptr<GEDS> gedsService, std::string bucketArg, std::string keyArg,
                 std::optional<std::string> metadataArg);

public:
  auto lockFile() const { return std::lock_guard(_fileMutex); }

  GEDSFileHandle() = delete;
  virtual ~GEDSFileHandle();

  virtual absl::StatusOr<size_t> size() const = 0;
  virtual size_t localStorageSize() const { return 0; }
  virtual size_t localMemorySize() const { return 0; }
  int64_t openCount() const;
  void increaseOpenCount();
  void decreaseOpenCount();

  virtual void notifyUnused();

  std::chrono::system_clock::time_point lastOpened() const;
  std::chrono::system_clock::time_point lastReleased() const;

  virtual bool isValid() const;
  virtual bool isRelocatable() const { return false; }
  virtual bool isWriteable() const { return false; }

  virtual absl::StatusOr<std::shared_ptr<GEDSFileHandle>> relocate();

  virtual std::optional<std::string> metadata() const;

  virtual absl::Status setMetadata(std::optional<std::string> metadata, bool seal = true);

  virtual absl::StatusOr<size_t> readBytes(uint8_t *bytes, size_t position, size_t length);

  virtual absl::Status writeBytes(const uint8_t *bytes, size_t position, size_t length);

  virtual absl::Status write(std::istream &stream, size_t position = 0,
                             std::optional<size_t> lengthOptional = std::nullopt);

  virtual absl::Status truncate(size_t targetSize);

  virtual absl::Status seal();

  virtual absl::StatusOr<GEDSFile> open();

  virtual absl::StatusOr<int> rawFd() const;

  size_t roundToNearestMultiple(size_t number, size_t factor) const;

  absl::Status download(std::shared_ptr<GEDSFileHandle> destination);
  virtual absl::StatusOr<size_t> downloadRange(std::shared_ptr<GEDSFileHandle> destination,
                                               size_t srcPosition, size_t length,
                                               size_t destPosition);
};
