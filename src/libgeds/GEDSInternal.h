/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GEDS_GEDSINTERNAL_H
#define GEDS_GEDSINTERNAL_H

#include <chrono>
#include <string>

namespace geds {
enum class ConnectionState : int { Disconnected = 0, Connected, Unknown };
enum class ServiceState : int { Stopped = 0, Running, Unknown };
enum class FileMode : int { ReadWrite = 0, ReadOnly = 1 };

std::string to_string(geds::ConnectionState state);

std::string to_string(geds::ServiceState state);

inline auto grpcDefaultDeadline() {
  return std::chrono::system_clock::now() + std::chrono::seconds(10); // NOLINT
}

} // namespace geds

#endif // GEDS_GEDSINTERNAL_H
