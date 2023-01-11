/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#ifndef GEDS_FILESYSTEM_H
#define GEDS_FILESYSTEM_H

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <string>

namespace geds::filesystem {

absl::StatusOr<int> createFile(const std::string &path);
absl::Status touchFile(const std::string &path);
absl::Status removeFile(const std::string &path);
absl::Status mkdir(const std::string &path);
std::string mktempdir(const std::string &name);
std::string tempFile(const std::string &folder, const std::string &prefix);
std::string tempFile(const std::string &prefix);
std::string tempFile();

} // namespace geds::filesystem

#endif
