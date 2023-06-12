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
  } else if (key == "pub_sub_enabled" && value == "true") {
    pubSubEnabled = true;
  } else {
    LOG_ERROR("Configuration " + key + " not supported (type: string).");
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
  } else if (key == "http_server_port") {
    if (value > INT16_MAX) {
      return absl::InvalidArgumentError("Value " + std::to_string(value) + " is out of range for " +
                                        key);
    }
    portHttpServer = value;
  } else if (key == "cache_block_size") {
    cacheBlockSize = value;
  } else if (key == "io_thread_pool_size") {
    io_thread_pool_size = value;
  } else if (key == "available_local_storage") {
    available_local_storage = value;
  } else if (key == "available_local_memory") {
    available_local_memory = value;
  } else if (key == "pub_sub_enabled") {
    pubSubEnabled = value != 0;
  } else if (key == "cache_objects_from_s3") {
    cache_objects_from_s3 = value != 0;
  } else if (key == "force_relocation_when_stopping") {
    force_relocation_when_stopping = value != 0;
  } else {
    LOG_ERROR("Configuration " + key + " not supported (type: signed/unsigned integer).");
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

absl::Status GEDSConfig::set(const std::string &key, double value) {
  if (key == "storage_spilling_fraction") {
    storage_spilling_fraction = value;
    return absl::OkStatus();
  }
  LOG_ERROR("Configuration " + key + " not supported (type: double).");
  return absl::NotFoundError("Key " + key + " not found.");
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
  LOG_ERROR("Configuration " + key + " not supported (type: string).");
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
  if (key == "io_thread_pool_size") {
    return io_thread_pool_size;
  }
  if (key == "available_local_storage") {
    return available_local_storage;
  }
  if (key == "available_local_memory") {
    return available_local_memory;
  }
  LOG_ERROR("Configuration " + key + " not supported (type: signed/unsigned integer).");
  return absl::NotFoundError("Key " + key + " not found.");
}

absl::StatusOr<int64_t> GEDSConfig::getSignedInt(const std::string &key) const {
  auto value = getUnsignedInt(key);
  if (value.ok()) {
    return (int64_t)*value;
  }
  return value.status();
}

absl::StatusOr<double> GEDSConfig::getDouble(const std::string &key) const {
  if (key == "storage_spilling_fraction") {
    return storage_spilling_fraction;
  }
  LOG_ERROR("Configuration " + key + " not supported (type: double).");
  return absl::NotFoundError("Key " + key + " (double) not found.");
}
