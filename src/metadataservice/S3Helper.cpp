/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "S3Helper.h"

#include <map>
#include <optional>

#include "Logging.h"
#include "Object.h"
#include "S3Endpoint.h"

absl::Status PopulateKVS(std::shared_ptr<geds::ObjectStoreConfig> config,
                         std::shared_ptr<MDSKVS> kvs) {
  // Ensure the bucket already exists
  {
    auto status = kvs->createBucket(config->bucket);
    if (!status.ok() && status.code() != absl::StatusCode::kAlreadyExists) {
      return status;
    }
  }
  auto bucket = kvs->getBucket(config->bucket);
  if (!bucket.ok()) {
    return bucket.status();
  }
  auto s3Endpoint = geds::s3::Endpoint(config->endpointURL, config->accessKey, config->secretKey);
  auto files = s3Endpoint.list(config->bucket, "/");
  if (!files.ok()) {
    LOG_ERROR("Unable to list s3 endpoint for ", config->bucket, ": ", files.status().message());
    return files.status();
  }
  for (const auto &f : *files) {
    if (f.isDirectory) {
      continue;
    }
    LOG_DEBUG("Adding: ", config->bucket, "/", f.key);
    auto objInfo = geds::ObjectInfo{
        .location = "s3://" + config->bucket + "/" + f.key,
        .size = f.size,
        .sealedOffset = f.size,
        .metadata = std::nullopt
    };
    auto status = (*bucket)->createObject(
        geds::Object{.id = geds::ObjectID{config->bucket, f.key}, .info = objInfo});
    if (!status.ok() && status.code() != absl::StatusCode::kAlreadyExists) {
      LOG_ERROR("Unable to create entry for ", config->bucket, "/", f.key, ": ", status.message());
      continue;
    }
  }
  return absl::OkStatus();
}
