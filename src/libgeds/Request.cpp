/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <regex>
#include <string>

#include "Request.h"
#include "absl/status/status.h"

namespace geds::request {

absl::StatusOr<RequestType> parseRequestType(const std::string &message) {
  if (message.starts_with("GET")) {
    return RequestType::GET;
  }
  return absl::InvalidArgumentError("Invalid request!");
}

absl::Status parseGetRequest(const std::string &request, std::string &bucket, std::string &key,
                             size_t &position, size_t &length) {
  static std::regex regex( //
      "GET ([a-z\\d][a-z\\d\\.\\-]+[a-z\\d])/(.+)\\nRANGE (\\d+) (\\d+)",
      std::regex_constants::ECMAScript);
  std::smatch m;
  std::regex_match(request, m, regex);
  if (m.empty()) {
    return absl::InvalidArgumentError("Unable to parse " + request);
  }
  bucket = m[0];
  key = m[1];
  position = std::stoull(m[2]);
  length = std::stoull(m[2]);
  return absl::OkStatus();
}

std::string createGetRequest(const std::string &bucket, const std::string &key, size_t position,
                             size_t length) {
  return "GET " + bucket + "/" + key + "\nRANGE " + std::to_string(position) + " " +
         std::to_string(length);
}
} // namespace geds::request
