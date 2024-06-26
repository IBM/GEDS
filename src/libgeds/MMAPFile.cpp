/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "MMAPFile.h"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <fcntl.h>
#include <iterator>
#include <optional>
#include <string>
#include <sys/mman.h>
#include <unistd.h>

#include "Filesystem.h"
#include "Logging.h"

#define CHECK_FILE_OPEN                                                                            \
  if (_fd < 0) {                                                                                   \
    return absl::UnavailableError("The file at " + _path + " is not open!");                       \
  }

namespace geds::filesystem {

static const size_t MMAP_pageSize = getpagesize();

MMAPFile::MMAPFile(std::string pathArg, bool overwrite) : _path(std::move(pathArg)) {
  auto mode = O_RDWR | O_CREAT;
  if (overwrite) {
    mode |= O_TRUNC;
  }

  // NOLINTNEXTLINE
  _fd = ::open(_path.c_str(), mode, S_IRUSR | S_IWUSR);
  if (_fd < 0) {
    int error = errno;
    auto message = "Unable to open " + _path + ". Reason: " + strerror(error);
    LOG_ERROR(message);
    throw std::runtime_error{message};
  }
}

MMAPFile::~MMAPFile() {
  auto lock = getWriteLock();
  release();
  if (_fd >= 0) {
    (void)::close(_fd);
    _fd = -1;

    auto removeStatus = removeFile(_path);
    if (!removeStatus.ok()) {
      LOG_ERROR("Unable to delete ", _path, " reason: ", removeStatus.message());
    }
  }
}

absl::Status MMAPFile::increaseMmap(size_t requestSize) {
  CHECK_FILE_OPEN;

  if (_mmapSize < requestSize) {
    size_t nPages = requestSize / MMAP_pageSize + (requestSize % MMAP_pageSize > 0 ? 1 : 0);
    size_t newSize = nPages * MMAP_pageSize;
    void *m = nullptr;

    int e = -1;
    do {
      e = posix_fallocate64(_fd, _mmapSize, newSize - _mmapSize);
    } while (e != 0 && errno == EINTR);
    if (e != 0) {
      e = errno;
      return absl::UnknownError("Unable to extend file: " + _path + ". Reason: " + strerror(e));
    }
    if (_mmapPtr == nullptr) {
      // fallocate, int mode, __off_t offset, __off_t len)
      m = mmap(nullptr, newSize, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0);
      if (m == MAP_FAILED) { // NOLINT
        return absl::UnknownError("Failed to map file " + _path + " with requested size " +
                                  std::to_string(requestSize) + ".");
      }
    } else {
      // TODO: We use MREMAP_MAYMOVE.
      m = mremap(_mmapPtr, _mmapSize, newSize, MREMAP_MAYMOVE);
      if (m == MAP_FAILED) { // NOLINT
        int e = errno;
        return absl::UnknownError("mremap for file " + _path + " failed. Reason: " + strerror(e));
      }
    }
    _mmapPtr = static_cast<uint8_t *>(m);
    _mmapSize = newSize;
  }
  return absl::OkStatus();
}

absl::StatusOr<size_t> MMAPFile::readBytes(uint8_t *bytes, size_t position, size_t length) {
  if (length > SSIZE_MAX) {
    return absl::FailedPreconditionError("Lengths > " + std::to_string(SSIZE_MAX) +
                                         " are not supported!");
  }
  if (position >= SSIZE_MAX) {
    return absl::FailedPreconditionError("Stream positions > " + std::to_string(SSIZE_MAX) +
                                         " are not supported!");
  }
  if (length == 0) {
    return 0;
  }

  // Reopen the file if it has been unmapped.
  auto status = reopen();
  if (!status.ok()) {
    return status;
  }

  auto lock = getReadLock();
  if (position >= _size) {
    return 0;
  }
  length = std::min(length, _size - position);
  if (length == 0) {
    return 0;
  }
  auto n = std::min(_size, position + length) - position;
  (void)std::memcpy(bytes, _mmapPtr + position, n); //
  return n;
}

absl::StatusOr<uint8_t *> MMAPFile::rawPtr() {
  auto status = reopen();
  if (!status.ok()) {
    return status;
  }
  return _mmapPtr;
}

absl::StatusOr<int> MMAPFile::rawFd() const {
  CHECK_FILE_OPEN;

  return _fd;
}

absl::Status MMAPFile::truncate(size_t targetSize) {
  auto lock = getWriteLock();
  if (targetSize > _size) {
    auto status = increaseMmap(targetSize);
    if (!status.ok()) {
      return status;
    }
  }
  _size = targetSize;
  return absl::OkStatus();
}

absl::Status MMAPFile::writeBytes(const uint8_t *bytes, size_t position, size_t length) {
  if (length == 0) {
    // Length is 0, return.
    return absl::OkStatus();
  }
  auto lock = getWriteLock();
  auto newSize = position + length;

  // Check if the new file size is bigger, if yes, increase the file size.
  if (newSize > _size) {
    auto status = increaseMmap(newSize);
    if (!status.ok()) {
      return status;
    }
  }
  // mmap is invalid.
  if (_mmapPtr == nullptr) {
    return absl::InternalError("The file is not mmapped!");
  }
  // NOLINTNEXTLINE
  (void)std::memcpy(_mmapPtr + position, bytes, length);
  if (newSize > _size) {
    _size = newSize;
  }
  return absl::OkStatus();
}

absl::StatusOr<size_t> MMAPFile::write(std::istream &stream, size_t position,
                                       std::optional<size_t> lengthArg) {
  size_t length = 0;
  if (!lengthArg.has_value()) {
    auto pos = stream.tellg();
    std::for_each(std::istream_iterator<char>(stream), std::istream_iterator<char>(),
                  [&length](char) { length += 1; });
    stream.seekg(pos);
  } else {
    length = lengthArg.value();
  }
  if (length == 0) {
    LOG_DEBUG("Stream is empty");
    return absl::OkStatus();
  }
  auto lock = getWriteLock();
  auto newSize = position + length;

  auto status = increaseMmap(newSize);
  if (!status.ok()) {
    return status;
  }

  // mmap is invalid.
  if (_mmapPtr == nullptr) {
    return absl::InternalError("The file is not mmapped!");
  }
  std::copy(std::istream_iterator<char>(stream), std::istream_iterator<char>(),
            reinterpret_cast<char *>(_mmapPtr + position));
  if (newSize > _size) {
    _size = newSize;
  }
  LOG_DEBUG("Wrote ", std::to_string(length), " to ", _path);
  return length;
}

void MMAPFile::release() {
  if (_mmapPtr != 0) {
    int err = munmap(_mmapPtr, _mmapSize);
    if (err != 0) {
      err = errno;
      LOG_ERROR("Unable to munmap ", _path, " reason: ", strerror(err));
    }
    _mmapPtr = nullptr;
    _mmapSize = 0;
  }
}

absl::Status MMAPFile::reopen() {
  if (_mmapPtr == nullptr) {
    auto lock = getWriteLock();
    if (_mmapPtr != nullptr) {
      return absl::OkStatus();
    }
    size_t nPages = _size / MMAP_pageSize + (_size % MMAP_pageSize > 0 ? 1 : 0);
    size_t mmapSize = nPages * MMAP_pageSize;
    // fallocate, int mode, __off_t offset, __off_t len)
    auto m = mmap(nullptr, mmapSize, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0);
    if (m == MAP_FAILED) { // NOLINT
      return absl::UnknownError("Failed to map file " + _path + " with requested size " +
                                std::to_string(_size) + ".");
    }
    _mmapPtr = static_cast<uint8_t *>(m);
    _mmapSize = mmapSize;
  }
  return absl::OkStatus();
}

absl::Status MMAPFile::fsync() const {
  CHECK_FILE_OPEN;

  int e = 0;
  do {
    e = ::fsync(_fd);
  } while (e != 0 && errno == EINTR);
  if (e != 0) {
    int err = errno;
    return absl::UnknownError("Unable to fsync " + _path + ": " + strerror(err));
  }
  return absl::OkStatus();
}

void MMAPFile::notifyUnused() {
  auto lock = getWriteLock();
  release();
}

} // namespace geds::filesystem
