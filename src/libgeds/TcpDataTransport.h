/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <thread>
#include <vector>

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/vector.hpp>

namespace geds {

size_t MAXIMUM_TCP_THREADS();

namespace tcp_transport {

enum class RequestType { GET };

absl::StatusOr<RequestType> parseRequestType(const std::string &message);

absl::Status parseGetRequest(const std::string &request, std::string &bucket, std::string &key,
                             size_t &position, size_t &length);
std::string createGetRequest(const std::string &bucket, const std::string &key, size_t position,
                             size_t length);

struct Response {
  int statusCode;
  size_t length;
};

} // namespace tcp_transport
}; // namespace geds
