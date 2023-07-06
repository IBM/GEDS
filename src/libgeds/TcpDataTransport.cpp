/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TcpDataTransport.h"

#include <regex>
#include <string>

#include "Logging.h"

namespace geds {
size_t MAXIMUM_TCP_THREADS() {
  return std::max<size_t>(8, std::thread::hardware_concurrency() * 2);
}

namespace tcp_transport {
absl::StatusOr<RequestType> parseRequestType(const std::string &message) {
  if (message.starts_with("GET")) {
    return RequestType::GET;
  }
  return absl::InvalidArgumentError("Invalid request!");
}

absl::Status parseGetRequest(const std::string &request, std::string &bucket, std::string &key,
                             size_t &offset, size_t &length) {
  LOG_DEBUG("Trying to parse ", request);

  static std::regex regex( //
      "GET ([a-z\\d][a-z\\d\\.\\-]+[a-z\\d])\\/(.+)[\\n]+RANGE (\\d+) (\\d+)\\D*",
      std::regex_constants::ECMAScript);
  std::smatch m;
  std::regex_match(request, m, regex);
  if (m.empty()) {
    return absl::InvalidArgumentError("Unable to parse '" + request + "'");
  }
  bucket = m[1];
  key = m[2];
  offset = std::stoull(m[3]);
  length = std::stoull(m[4]);
  return absl::OkStatus();
}

std::string createGetRequest(const std::string &bucket, const std::string &key, size_t offset,
                             size_t length) {
  std::stringstream ss;
  ss << "GET " << bucket << "/" << key << "\nRANGE " << offset << " " << length;
  return ss.str();
}

} // namespace tcp_transport
} // namespace geds
