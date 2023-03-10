/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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

namespace geds::filesystem {

class LocalFile {
  const std::string _path;

  int _fd{-1};

  size_t _size{0};

  /**
   * @brief Seek commands require locking of the file.
   */
  mutable std::recursive_mutex __mutex;
  auto getLock() const { return std::lock_guard(__mutex); }

protected:
  absl::StatusOr<size_t> fileSize() const;

public:
  LocalFile() = delete;
  LocalFile(LocalFile &) = delete;
  LocalFile &operator=(LocalFile &) = delete;

  LocalFile(std::string path, bool overwrite = true);
  ~LocalFile();

  [[nodiscard]] const std::string &path() const { return _path; }

  [[nodiscard]] size_t size() const { return _size; }

  absl::StatusOr<int> rawFd() const;

  absl::StatusOr<size_t> readBytes(uint8_t *bytes, size_t position, size_t length);

  absl::Status truncate(size_t targetSize);

  absl::Status writeBytes(const uint8_t *bytes, size_t position, size_t length);
  absl::StatusOr<size_t> write(std::istream &stream, size_t position,
                               std::optional<size_t> length = std::nullopt);

  static const std::string statisticsLabel() { return "LocalFile"; }
};

} // namespace geds::filesystem
