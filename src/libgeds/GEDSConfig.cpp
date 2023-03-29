/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "GEDSConfig.h"
#include "Logging.h"

absl::Status GEDSConfig::set(const std::string &key, const std::string &value) {
  LOG_DEBUG("Trying to set '", key, "' to '", value, "'");
  if (key == "listen_address") {
    listenAddress = value;
  } else if (key == "hostname") {
    hostname = value == "" ? std::nullopt : std::make_optional(value);
  } else if (key == "local_storage_path") {
    localStoragePath = value;
  } else {
    return absl::NotFoundError("Key " + key + " not found.");
  }
  LOG_INFO("Set '", key, "' to '", value, "'");
  return absl::OkStatus();
}

absl::Status GEDSConfig::set(const std::string &key, size_t value) {
  LOG_DEBUG("Trying to set '", key, "' to '", value, "'");
  if (value == 0) {
    return absl::InvalidArgumentError("Value " + std::to_string(value) + " is out of range for " +
                                      key);
  }
  if (key == "port") {
    if (value > INT16_MAX) {
      return absl::InvalidArgumentError("Value " + std::to_string(value) + " is out of range for " +
                                        key);
    }
    port = value;
  } else if ("http_server_port") {
    if (value > INT16_MAX) {
      return absl::InvalidArgumentError("Value " + std::to_string(value) + " is out of range for " +
                                        key);
    }
    portHttpServer = value;
  } else if (key == "cache_block_size") {
    cacheBlockSize = value;
  } else {
    return absl::NotFoundError("Key " + key + " not found.");
  }
  LOG_INFO("Set '", key, "' to '", value, "'");
  return absl::OkStatus();
}

absl::Status GEDSConfig::set(const std::string &key, int64_t value) {
  if (value < 0) {
    return absl::InvalidArgumentError("Value " + std::to_string(value) + " is out of range for " +
                                      key);
  }
  return set(key, (size_t)value);
}

absl::StatusOr<std::string> GEDSConfig::getString(const std::string &key) const {
  LOG_INFO("Get ", key, " as string");
  if (key == "listen_address") {
    return listenAddress;
  }
  if (key == "hostname") {
    return hostname.value_or("");
  }
  if (key == "local_storage_path") {
    return localStoragePath;
  }
  return absl::NotFoundError("Key " + key + " not found.");
}

absl::StatusOr<size_t> GEDSConfig::getUnsignedInt(const std::string &key) const {
  LOG_INFO("Get ", key, " as integer");
  if (key == "port") {
    return port;
  }
  if (key == "http_server_port") {
    return portHttpServer;
  }
  if (key == "cache_block_size") {
    return cacheBlockSize;
  }
  return absl::NotFoundError("Key " + key + " not found.");
}

absl::StatusOr<int64_t> GEDSConfig::getSignedInt(const std::string &key) const {
  auto value = getUnsignedInt(key);
  if (value.ok()) {
    return (int64_t)*value;
  }
  return value.status();
}
