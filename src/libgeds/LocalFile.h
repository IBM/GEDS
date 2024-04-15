/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <vector>

#include <absl/status/status.h>
#include <absl/status/statusor.h>

#include "RWConcurrentObjectAdaptor.h"

namespace geds::filesystem {

class LocalFile {
  const std::string _path;

  int _fd{-1};

  std::atomic<size_t> _size{0};

  /**
   * @brief Seek commands require locking of the file.
   */
  mutable std::recursive_mutex __mutex;

protected:
  absl::StatusOr<size_t> fileSize() const;

public:
  LocalFile() = delete;
  LocalFile(LocalFile &) = delete;
  LocalFile &operator=(LocalFile &) = delete;

  LocalFile(std::string path, bool overwrite = true);
  ~LocalFile();

  [[nodiscard]] const std::string &path() const { return _path; }

  void notifyUnused();

  absl::Status fsync() const;

  [[nodiscard]] size_t size() const { return _size; }
  [[nodiscard]] size_t localStorageSize() const { return _size; }
  [[nodiscard]] size_t localMemorySize() const { return 0; }

  absl::StatusOr<int> rawFd() const;

  absl::StatusOr<uint8_t *> rawPtr() const;

  absl::StatusOr<size_t> readBytes(uint8_t *bytes, size_t position, size_t length);

  absl::Status truncate(size_t targetSize);

  absl::Status writeBytes(const uint8_t *bytes, size_t position, size_t length);
  absl::StatusOr<size_t> write(std::istream &stream, size_t position,
                               std::optional<size_t> length = std::nullopt);

  static const std::string statisticsLabel() { return "LocalFile"; }
};

} // namespace geds::filesystem
