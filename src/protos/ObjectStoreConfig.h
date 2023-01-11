/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#ifndef GEDS_OBJECT_STORE_CONFIG_H
#define GEDS_OBJECT_STORE_CONFIG_H

#include <string>
#include <utility>

namespace geds {

struct ObjectStoreConfig {
  const std::string bucket;
  const std::string endpointURL;
  const std::string accessKey;
  const std::string secretKey;
  ObjectStoreConfig(std::string ArgBucket, std::string ArgEndpointURL, std::string ArgAccessKey,
                    std::string ArgSecretKey)
      : bucket(std::move(ArgBucket)), endpointURL(std::move(ArgEndpointURL)),
        accessKey(std::move(ArgAccessKey)), secretKey(std::move(ArgSecretKey)) {}

  bool operator==(const ObjectStoreConfig &other) const {
    return bucket == other.bucket && endpointURL == other.endpointURL &&
           accessKey == other.accessKey && secretKey == other.secretKey;
  }
  bool operator!=(const ObjectStoreConfig &other) const { return !(*this == other); }
};
} // namespace geds

#endif
