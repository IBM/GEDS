/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#ifndef GEDS_MMAP_FILE_H
#define GEDS_MMAP_FILE_H

#include <absl/status/status.h>
#include <absl/status/statusor.h>

#include <cstddef>
#include <cstdint>
#include <istream>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <vector>

namespace geds::filesystem {

class MMAPFile {
  std::shared_mutex _mutex;
  auto getReadLock() { return std::shared_lock<std::shared_mutex>(_mutex); }
  auto getWriteLock() { return std::unique_lock<std::shared_mutex>(_mutex); }

  const std::string _path;

  int _fd{-1};
  size_t _size{0};

  size_t _mmapSize{0};
  uint8_t *_mmapPtr{nullptr};

  absl::Status increaseMmap(size_t requestSize);

public:
  MMAPFile() = delete;
  MMAPFile(MMAPFile &) = delete;
  MMAPFile &operator=(MMAPFile &) = delete;

  MMAPFile(std::string path, bool overwrite = true);
  ~MMAPFile();

  [[nodiscard]] const std::string &path() const { return _path; }

  [[nodiscard]] size_t size() const { return _size; }
  absl::StatusOr<const uint8_t *> rawPtr() const;

  absl::StatusOr<int> rawFd() const;

  absl::StatusOr<size_t> readBytes(uint8_t *bytes, size_t position, size_t length);

  absl::Status truncate(size_t targetSize);

  absl::Status writeBytes(const uint8_t *bytes, size_t position, size_t length);
  absl::StatusOr<size_t> write(std::istream &stream, size_t position,
                               std::optional<size_t> length = std::nullopt);

  static const std::string statisticsLabel() { return "MMAPFile"; }
};

} // namespace geds::filesystem

#endif
