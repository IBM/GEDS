/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <thread>

#include "Ports.h"

enum class GEDSNodeType { Standard, Storage };

struct GEDSConfig {
  /**
   * @brief The hostname of the metadata service/
   *
   * Format: `{HOSTNAME/IP}:{PORT}`
   */
  std::string metadataServiceAddress;

  /**
   * @brief GEDS listening address.
   */
  std::string listenAddress = "0.0.0.0";

  /**
   * @brief Hostname/IP used by GEDS to announce itself.
   *
   * If null, the hostname is determined by querying the metadata service.
   */
  std::optional<std::string> hostname = std::nullopt;

  /**
   * @brief GEDS listening port.
   */
  uint16_t port = defaultGEDSPort;

  /**
   * @brief GEDS web server port.
   *
   * E.g. prometheus endpoint.
   */
  uint16_t portHttpServer = defaultPrometheusPort;

  /**
   * @brief Storage path for files create by GEDS.
   *
   * The Postfix XXXXXX indicates that the path can be randomized.
   */
  std::string localStoragePath = "/tmp/GEDS_XXXXXX";

  /**
   * @brief Block size used for caching.
   *
   * Hadoop S3A uses 32MB - thus we set the same value.
   */
  size_t cacheBlockSize = 32 * 1024 * 1024;

  /**
   * @brief Relocate files on GEDS::close.
   */
  bool relocate_on_close = false;

  /**
   * @brief Size of I/O thread pool.
   */
  size_t io_thread_pool_size = std::max(std::thread::hardware_concurrency() / 2, (uint32_t)8);

  /**
   * @brief Available local storage.
   */
  size_t available_local_storage = 100 * 1024 * 1024 * (size_t)1024;

  size_t available_local_memory = 16 * 1024 * 1024 * (size_t)1024;

  /**
   * @brief Fraction of the storage where GEDS should start spilling.
   */
  double storage_spilling_fraction = 0.7;

  /**
   * @brief Node type.
   */
  GEDSNodeType node_type = GEDSNodeType::Standard;

  /**
   * @brief Publish/Subscribe is enabled.
   */
  bool pubSubEnabled = false;

  /**
   * @brief Force relocation when stopping.
   */
  bool force_relocation_when_stopping = false;

  GEDSConfig(std::string metadataServiceAddressArg)
      : metadataServiceAddress(std::move(metadataServiceAddressArg)) {
    if (available_local_storage <= 4 * 1024 * 1024 * (size_t)1024) {
      io_thread_pool_size = std::min<size_t>(io_thread_pool_size, 6);
      storage_spilling_fraction = 0.9;
    }
  }

  absl::Status set(const std::string &key, const std::string &value);
  absl::Status set(const std::string &key, size_t value);
  absl::Status set(const std::string &key, int64_t value);
  absl::Status set(const std::string &key, double value);

  absl::StatusOr<std::string> getString(const std::string &key) const;
  absl::StatusOr<size_t> getUnsignedInt(const std::string &key) const;
  absl::StatusOr<int64_t> getSignedInt(const std::string &key) const;
  absl::StatusOr<double> getDouble(const std::string &key) const;
};
