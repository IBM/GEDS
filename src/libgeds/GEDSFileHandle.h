/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GEDS_GEDSFILEHANDLE_H
#define GEDS_GEDSFILEHANDLE_H

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <memory>
#include <mutex>
#include <optional>
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

  /**
   * @brief Mutex to protect rw operations.
   */
  mutable std::recursive_mutex __mutex;
  auto lockFile() const { return std::lock_guard(__mutex); }

protected:
  std::atomic<int64_t> _openCount{0};
  std::chrono::system_clock::time_point _lastOpened;
  std::chrono::system_clock::time_point _lastReleased;

  std::optional<std::string> _metadata;

  std::shared_ptr<GEDS> _gedsService;

  std::shared_ptr<GEDSFileHandle> getPtr() { return shared_from_this(); }

protected:
  // Constructors are private to enable `shared_from_this`.
  GEDSFileHandle(std::shared_ptr<GEDS> gedsService, std::string bucketArg, std::string keyArg,
                 std::optional<std::string> metadataArg);

public:
  GEDSFileHandle() = delete;
  virtual ~GEDSFileHandle();

  virtual absl::StatusOr<size_t> size() const = 0;
  int64_t openCount() const;
  void increaseOpenCount();
  void decreaseOpenCount();

  virtual void notifyUnused();

  std::chrono::system_clock::time_point lastOpened() const;
  std::chrono::system_clock::time_point lastReleased() const;

  virtual bool isValid() const = 0;
  virtual bool isWriteable() const { return false; }

  size_t roundToNearestMultiple(size_t number, size_t factor) const;

  const std::optional<std::string> &metadata() const;
  virtual absl::Status setMetadata(std::optional<std::string> metadata, bool seal = true);

  /**
   * @brief Read length bytes from file handle (or until EOF) at position into buffer. The API needs
   * to handle POSIX errors.
   * @return Effective number of bytes read.
   */
  virtual absl::StatusOr<size_t> readBytes(uint8_t *bytes, size_t position, size_t length);

  virtual absl::StatusOr<int> rawFd() const;

  /**
   * @brief Write length bytes into file handle at position. The API needs to handle POSIX errors.
   */
  virtual absl::Status writeBytes(const uint8_t *bytes, size_t position, size_t length);

  virtual absl::Status write(std::istream &stream, size_t position = 0,
                             std::optional<size_t> lengthOptional = std::nullopt);

  absl::Status download(std::shared_ptr<GEDSFileHandle> destination);
  virtual absl::StatusOr<size_t> downloadRange(std::shared_ptr<GEDSFileHandle> destination,
                                               size_t srcPosition, size_t length,
                                               size_t destPosition);

  virtual absl::Status truncate(size_t targetSize);

  virtual absl::Status seal();

  virtual absl::StatusOr<GEDSFile> open();
};

#endif // GEDS_GEDSFILEHANDLE_H
