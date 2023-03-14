/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "Ports.h"

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

  GEDSConfig(std::string metadataServiceAddressArg)
      : metadataServiceAddress(metadataServiceAddressArg) {}
};
