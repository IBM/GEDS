/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "LocalFile.h"

#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <ios>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include "Logging.h"

#define CHECK_FILE_OPEN                                                                            \
  if (_fd < 0) {                                                                                   \
    return absl::UnavailableError("The file at " + _path + " is not open!");                       \
  }

namespace geds::filesystem {
LocalFile::LocalFile(std::string pathArg, bool overwrite) : _path(std::move(pathArg)) {
  auto mode = O_RDWR | O_CREAT;
  if (overwrite) {
    mode |= O_TRUNC;
  }

  // NOLINTNEXTLINE
  _fd = ::open(_path.c_str(), mode, S_IRUSR | S_IWUSR);
  if (_fd < 0) {
    int error = errno;
    throw std::runtime_error{"Unable to open " + _path + ". Reason: " + strerror(error)};
  }
  auto size = fileSize();
  if (!size.ok()) {
    throw std::runtime_error{"Unable to determine size on " + _path +
                             ". Reason: " + std::string{size.status().message()}};
  }
  _size = *size;
}

LocalFile::~LocalFile() {
  if (_fd >= 0) {
    (void)::close(_fd);
  }
  _fd = -1;
}

absl::StatusOr<size_t> LocalFile::fileSize() const {
  CHECK_FILE_OPEN
  auto lock = getLock();
  if (fsync(_fd) != 0) {
    int error = errno;
    auto message = "Fsync on " + _path + " reported: " + strerror(error);
    LOG_ERROR(message);
    return absl::UnknownError(message);
  }

  struct stat statBuf {};
  if (fstat(_fd, &statBuf) != 0) {
    int error = errno;
    auto message = "Fstat on " + _path + " reported: " + strerror(error);
    LOG_ERROR(message);
    return absl::UnknownError(message);
  }
  return statBuf.st_size;
}

absl::StatusOr<int> LocalFile::rawFd() const {
  CHECK_FILE_OPEN
  return _fd;
}

absl::StatusOr<size_t> LocalFile::readBytes(uint8_t *bytes, size_t position, size_t length) {
  if (position >= INT64_MAX) {
    return absl::FailedPreconditionError("Stream positions > " + std::to_string(INT64_MAX) +
                                         " are not supported!");
  }
  if (length == 0) {
    return 0;
  }
  auto lock = getLock();

  if (position > _size) {
    return 0;
  }
  length = std::min(length, _size - position);
  if (length == 0) {
    return 0;
  }

  auto off = ::lseek64(_fd, position, SEEK_SET);
  if (off == -1) {
    int err = errno;
    auto errorMessage = "lseek on " + _path + " failed: " + strerror(err);
    LOG_ERROR(errorMessage);
    return absl::UnknownError(errorMessage);
  }

  size_t offset = 0;
  while (offset < length) {
    size_t count = length - offset;

    // Truncate length to conform to POSIX API:
    // See https://pubs.opengroup.org/onlinepubs/009695399/functions/read.html
    if (count > SSIZE_MAX) {
      count = SSIZE_MAX;
    }

    // Loop and check for EINTR.
    ssize_t numBytes = 0;
    do {
      numBytes = ::read(_fd, &bytes[offset], count);
    } while (numBytes == -1 && errno == EINTR);

    // Error is unrecoverable.
    if (numBytes < 0) {
      int err = errno;
      auto errorMessage = "Error reading " + _path + ": " + strerror(err);
      LOG_ERROR(errorMessage);
      return absl::UnknownError(errorMessage);
    }
    offset += numBytes;

    // Encountered an EOF.
    if (numBytes == 0) {
      break;
    }
  }
  return offset;
}

absl::Status LocalFile::truncate(size_t targetSize) {
  CHECK_FILE_OPEN

  auto lock = getLock();
  int e = ftruncate64(_fd, targetSize);
  if (e < 0) {
    int err = errno;
    std::string errorMessage = "Unable to ftruncate file " + _path + ": " + strerror(err);
    LOG_ERROR(errorMessage);
    return absl::UnknownError(errorMessage);
  }
  _size = targetSize;
  return absl::OkStatus();
}

absl::Status LocalFile::writeBytes(const uint8_t *bytes, size_t position, size_t length) {
  if (position > INT64_MAX) {
    return absl::FailedPreconditionError("Stream positions > " + std::to_string(position) +
                                         " are not yet supported.");
  }
  if (length == 0) {
    return absl::OkStatus();
  }

  auto lock = getLock();
  if (position > 0 && _size < position) {
    auto status = truncate(position);
    if (!status.ok()) {
      return status;
    }
  }

  auto seek = ::lseek64(_fd, position, SEEK_SET);
  if (seek < 0) {
    int err = errno;
    std::string errorMessage = "lseek on " + _path + " failed due to " + strerror(err) + ".";
    LOG_ERROR(errorMessage);
    return absl::UnknownError(errorMessage);
  }

  size_t offset = 0;
  while (offset < length) {
    size_t count = length - offset;

    // Truncate length to conform to POSIX API:
    // See https://pubs.opengroup.org/onlinepubs/009695399/functions/write.html
    if (count > SSIZE_MAX) {
      count = SSIZE_MAX;
    }

    // Loop and check for EINTR.
    ssize_t numBytes = 0;
    do {
      numBytes = ::write(_fd, &bytes[offset], count);
    } while (numBytes == -1 && errno == EINTR);
    // Error is unrecoverable.
    if (numBytes < 0) {
      int err = errno;
      std::string errorMessage = "Error writing " + _path + ": " + strerror(err);
      LOG_ERROR(errorMessage);
      return absl::UnknownError(errorMessage);
    }
    if (numBytes == 0) {
      std::string errorMessage = "Write on " + _path + " returned an EOF.";
      LOG_ERROR(errorMessage);
      return absl::UnknownError(errorMessage);
    }
    offset += numBytes;
  }
  _size = std::max(_size, position + offset);
  return absl::OkStatus();
}
absl::StatusOr<size_t> LocalFile::write(std::istream &stream, size_t position,
                                        std::optional<size_t> lengthOpt) {
  auto buffer = std::vector<char>(4096, 0);
  auto length = lengthOpt.value_or(INT64_MAX);

  size_t n = 0;
  std::streamsize count;
  do {
    auto maxRead = std::min(std::min(buffer.size(), length - n), (size_t)LONG_MAX);
    count = stream.readsome(buffer.data(), (long)maxRead);
    if (count < 0) {
      return absl::UnknownError("Unable to read from stream");
    }
    auto status = writeBytes(reinterpret_cast<uint8_t *>(buffer.data()), position + n, count);
    if (!status.ok()) {
      return status;
    }
    n += count;
  } while (count != 0);
  return n;
}
} // namespace geds::filesystem
