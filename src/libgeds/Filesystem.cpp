/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#include "Filesystem.h"

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <unistd.h>

#include "Logging.h"

namespace geds {
namespace filesystem {

absl::StatusOr<int> createFile(const std::string &path) {
  mode_t mode = S_IRUSR | S_IWUSR;
  int fd = open(path.c_str(), O_CREAT | O_EXCL, mode);
  if (fd < 0 && (errno != EEXIST)) {
    int error = errno;
    return absl::UnknownError("Unable to create file " + path + ": " + std::strerror(error));
  }
  return fd;
}

absl::Status touchFile(const std::string &path) {
  auto fileStatus = createFile(path);
  if (!fileStatus.ok()) {
    return fileStatus.status();
  }
  (void)close(fileStatus.value());
  return absl::OkStatus();
}

absl::Status removeFile(const std::string &path) {
  int err = unlink(path.c_str());
  if (err != 0 && (errno != ENOENT)) {
    int error = errno;
    return absl::UnknownError("Unable to delete file " + path + ": " + std::strerror(error));
  }
  return absl::OkStatus();
}

absl::Status mkdir(const std::string &path) {
  std::error_code errorCode;
  auto fsPath = std::filesystem::path(path);
  if (!std::filesystem::is_directory(fsPath)) {
    bool success = std::filesystem::create_directory(fsPath, errorCode);
    if (!success && errorCode.value() != 0) {
      return absl::UnknownError("Unable to create directory " + path +
                                "' Reason: " + errorCode.message());
    }
  }
  return absl::OkStatus();
}

std::string mktempdir(const std::string &name) {
  auto path = name;
  if (!path.ends_with("XXXXXX")) {
    path += "XXXXXX";
  }
  char *r = mkdtemp(path.data());
  if (r == NULL) {
    int err = errno;
    auto errorMessage = "mkdtemp returned an error while trying to create a tempdir with pattern " +
                        path + ": " + std::strerror(err);
    LOG_ERROR << errorMessage << std::endl;
    throw std::runtime_error(errorMessage);
  }
  return path;
}

std::string tempFile() {
  auto prefix = "GEDS_tempfile";
  return tempFile(prefix);
}

std::string tempFile(const std::string &prefix) {
  auto folder = std::filesystem::temp_directory_path();
  return tempFile(folder, prefix);
}

std::string tempFile(const std::string &folder, const std::string &prefix) {
  auto path = folder + "/" + prefix + "XXXXXX";

  std::string tmp = path + "XXXXXX";
  int fd = mkstemp(tmp.data());
  if (fd < 0) {
    int error = errno;
    auto errorMessage =
        "mkstemp returned an invalid file descriptor for " + prefix + ": " + std::strerror(error);
    LOG_ERROR << errorMessage << std::endl;
    throw std::runtime_error(errorMessage);
  }
  (void)close(fd);
  return tmp;
}

} // namespace filesystem
} // namespace geds
