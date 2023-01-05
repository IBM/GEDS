/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GEDS_GEDSFILE_H
#define GEDS_GEDSFILE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <absl/status/status.h>
#include <absl/status/statusor.h>

#include "GEDSInternal.h"

class GEDSFileHandle;

/**
 * @brief GEDS File abstraction. Exposes a file buffer.
 *
 * - Implements Sparse-File semantics
 * - Sparse File for caching.
 * - Use GEDSFile everywhere to allow unseal.
 */
class GEDSFile {
protected:
  std::shared_ptr<GEDSFileHandle> _fileHandle;

  virtual absl::StatusOr<size_t> readBytes(uint8_t *bytes, size_t position, size_t length);
  virtual absl::Status writeBytes(const uint8_t *bytes, size_t position, size_t length);

public:
  GEDSFile() = delete;
  GEDSFile(std::shared_ptr<GEDSFileHandle> fileHandle);
  GEDSFile(const GEDSFile &other);
  GEDSFile(GEDSFile &&other);
  GEDSFile &operator=(const GEDSFile &other);
  GEDSFile &operator=(GEDSFile &&other);
  bool operator==(const GEDSFile &other) const;
  virtual ~GEDSFile();

  [[nodiscard]] size_t size() const;

  [[nodiscard]] const std::string &bucket() const;
  [[nodiscard]] const std::string &key() const;
  [[nodiscard]] const std::string &identifier() const;
  [[nodiscard]] const std::shared_ptr<GEDSFileHandle> fileHandle() const;
  [[nodiscard]] bool isWriteable() const;

  template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
  absl::StatusOr<size_t> read(T *buffer, size_t position, size_t length) {
    size_t lengthBytes = length * sizeof(T);
    size_t positionBytes = position * sizeof(T);

    // NOLINTNEXTLINE
    auto readStatus = readBytes(reinterpret_cast<uint8_t *>(buffer), positionBytes, lengthBytes);
    if (!readStatus.ok()) {
      return readStatus.status();
    }
    return readStatus.value() / sizeof(T);
  }

  template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
  absl::StatusOr<size_t> read(std::vector<T> &buffer, size_t offset, size_t position,
                              size_t length) {
    size_t requiredLength = offset + length;
    auto prevBufferSize = buffer.size();
    if (prevBufferSize < requiredLength) {
      buffer.resize(requiredLength);
    }
    auto readStatus = read(&buffer[offset], position, length);
    if (!readStatus.ok()) {
      return readStatus.status();
    }
    auto readLength = readStatus.value();
    if (prevBufferSize < offset + readLength) {
      buffer.resize(offset + readLength);
    }
    return readLength;
  }

  template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
  absl::StatusOr<size_t> read(std::vector<T> &buffer, size_t position, size_t length) {
    return read(buffer, 0, position, length);
  }

  template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
  absl::Status write(const T *buffer, size_t position, size_t length) {
    size_t positionBytes = position * sizeof(T);
    size_t lengthBytes = length * sizeof(T);
    // NOLINTNEXTLINE
    return writeBytes(reinterpret_cast<const uint8_t *>(buffer), positionBytes, lengthBytes);
  }

  template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
  absl::Status write(const std::vector<T> &buffer, size_t offset, size_t position, size_t length) {
    if (offset > buffer.size()) {
      return absl::FailedPreconditionError("Offset is bigger than buffer.");
    }
    if (offset + length > buffer.size()) {
      return absl::FailedPreconditionError("Invalid length!");
    }
    return write(&buffer[offset], position, length);
  }

  template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
  absl::Status write(const std::vector<T> &buffer, size_t position, size_t length) {
    return write(buffer, 0, position, length);
  }

  absl::Status truncate(size_t size);

  absl::StatusOr<int> rawFd() const;

  absl::StatusOr<uint8_t *> rawPtr() const;

  absl::Status copyTo(GEDSFile &destination) const;

  absl::Status seal();
};

#endif // GEDS_GEDSFILE_H
