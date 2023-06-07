/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string>

#include <absl/status/status.h>
#include <absl/status/statusor.h>

namespace geds::request {

enum class RequestType { GET };

absl::StatusOr<RequestType> parseRequestType(const std::string &message);

absl::Status parseGetRequest(const std::string &request, std::string &bucket, std::string &key,
                             size_t &position, size_t &length);
std::string createGetRequest(const std::string &bucket, const std::string &key, size_t position,
                             size_t length);
} // namespace geds::request
