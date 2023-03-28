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

class MMAPFile : public utility::RWConcurrentObjectAdaptor {
  const std::string _path;

  int _fd{-1};
  size_t _size{0};

  size_t _mmapSize{0};
  uint8_t *_mmapPtr{nullptr};

  std::atomic<size_t> _ioProcesses;

  absl::Status increaseMmap(size_t requestSize);

  absl::Status reopen();
  void release();

public:
  MMAPFile() = delete;
  MMAPFile(MMAPFile &) = delete;
  MMAPFile &operator=(MMAPFile &) = delete;

  MMAPFile(std::string path, bool overwrite = true);
  ~MMAPFile();

  void notifyUnused();

  [[nodiscard]] const std::string &path() const { return _path; }

  [[nodiscard]] size_t size() const { return _size; }

  absl::StatusOr<uint8_t *> rawPtr();

  absl::StatusOr<int> rawFd() const;

  absl::StatusOr<size_t> readBytes(uint8_t *bytes, size_t position, size_t length);

  absl::Status truncate(size_t targetSize);

  absl::Status writeBytes(const uint8_t *bytes, size_t position, size_t length);
  absl::StatusOr<size_t> write(std::istream &stream, size_t position,
                               std::optional<size_t> length = std::nullopt);

  static const std::string statisticsLabel() { return "MMAPFile"; }
};

} // namespace geds::filesystem
