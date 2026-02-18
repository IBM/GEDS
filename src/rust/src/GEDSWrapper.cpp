/**
* Copyright 2022- IBM Inc. All rights reserved
* SPDX-License-Identifier: Apache-2.0
*/

#include "GEDSWrapper.h"
#include "GEDS.h"
#include "PubSub.h"
#include "GEDSConfig.h"
#include "geds_rs/src/lib.rs.h"
#include "geds.grpc.pb.h"

namespace geds_rs {
  std::unique_ptr<GEDSWrapper> new_wrapper(const shared::GEDSConfig &sharedConfig) {
    return std::make_unique<GEDSWrapper>(sharedConfig);
  }

  GEDSWrapper::GEDSWrapper(const shared::GEDSConfig &sharedConfig) {
    auto config = GEDSConfig(std::string(sharedConfig.metadata_service_address));
    config.listenAddress = std::string(sharedConfig.listen_address);
    config.port = sharedConfig.port;
    config.portHttpServer = sharedConfig.port_http_server;
    config.localStoragePath = std::string(sharedConfig.local_storage_path);
    config.cacheBlockSize = sharedConfig.cache_block_size;
    config.cache_objects_from_s3 = sharedConfig.cache_objects_from_s3;
    config.available_local_storage = sharedConfig.available_local_storage;
    config.available_local_memory = sharedConfig.available_local_memory;
    config.force_relocation_when_stopping = sharedConfig.force_relocation_when_stopping;
    config.pubSubEnabled = sharedConfig.pub_sub_enabled;

    if (sharedConfig.hostname == "null") {
    	  config.hostname = std::nullopt;
    }
    gedsPtr = GEDS::factory(config);
  }

  shared::Status GEDSWrapper::start() const {
    const auto status = gedsPtr->start();
    return shared::Status{.message = status.ToString(), .ok = status.ok()};
  }

  shared::Status GEDSWrapper::stop() const {
    const auto status = gedsPtr->stop();
    return shared::Status{.message = status.ToString(), .ok = status.ok()};
  }

  shared::StatusOrGEDSFileWrapper GEDSWrapper::create(const rust::Str bucket, const rust::Str key,
                                                      const bool overwrite) const {
    auto status = gedsPtr->create(std::string(bucket), std::string(key), overwrite);
    std::shared_ptr<GEDSFileWrapper> value;
    if (status.ok()) {
      value = std::make_shared<GEDSFileWrapper>(*status);
    }
    return {
      .status = {.message = status.status().ToString(), .ok = status.ok()},
      .value = value
    };
  }

  shared::Status GEDSWrapper::create_bucket(const rust::Str bucket) const {
    const auto status = gedsPtr->createBucket(std::string(bucket));
    return shared::Status{.message = status.ToString(), .ok = status.ok()};
  }

  shared::Status GEDSWrapper::mkdirs(const rust::Str bucket, const rust::Str path) const {
    const auto status = gedsPtr->mkdirs(std::string(bucket), std::string(path));
    return shared::Status{.message = status.ToString(), .ok = status.ok()};
  }

  rust::Vec<shared::GEDSFileStatus> to_rust_vec(const std::vector<GEDSFileStatus> &stdv) {
    rust::Vec<shared::GEDSFileStatus> rustv;

    for (const auto &[key, size, is_directory]: stdv) {
      rustv.emplace_back(shared::GEDSFileStatus{.key = key, .size = size, .is_directory = is_directory});
    }

    return rustv;
  }

  shared::StatusOrVecGEDSFileStatus
  GEDSWrapper::list(const rust::Str bucket, const rust::Str key) const {
    auto status = gedsPtr->list(std::string(bucket), std::string(key));
    rust::Vec<shared::GEDSFileStatus> rustVec;
    if (status.ok()) {
      rustVec = to_rust_vec(*status);
    }
    return {.status = {.message = status.status().ToString(), .ok = status.ok()}, .value = rustVec};
  }

  shared::StatusOrVecGEDSFileStatus GEDSWrapper::list_folder(const rust::Str bucket,
                                                             const rust::Str prefix) const {
    auto status = gedsPtr->listAsFolder(std::string(bucket), std::string(prefix));
    rust::Vec<shared::GEDSFileStatus> rustVec;
    if (status.ok()) {
      rustVec = to_rust_vec(*status);
    }
    return {.status = {.message = status.status().ToString(), .ok = status.ok()}, .value = rustVec};
  }

  shared::StatusOrGEDSFileStatus GEDSWrapper::status(const rust::Str bucket, const rust::Str key) const {
    const auto status = gedsPtr->status(std::string(bucket), std::string(key));
    shared::GEDSFileStatus fileStatus;
    if (status.ok()) {
      fileStatus.is_directory = status->isDirectory;
      fileStatus.key = status->key;
      fileStatus.size = status->size;
    }
    return {.status = {.message = status.status().ToString(), .ok = status.ok()}, .value = fileStatus};
  }

  auto GEDSWrapper::open(const rust::Str bucket, const rust::Str key) const -> shared::StatusOrGEDSFileWrapper {
    const auto status = gedsPtr->open(std::string(bucket), std::string(key));
    std::shared_ptr<GEDSFileWrapper> value;
    if (status.ok()) {
      value = std::make_shared<GEDSFileWrapper>(*status);
    }
    return {
      .status = {.message = status.status().ToString(), .ok = status.ok()},
      .value = value
    };
  }

  shared::Status GEDSWrapper::delete_object(const rust::Str bucket, const rust::Str key) const {
    const auto status = gedsPtr->deleteObject(std::string(bucket), std::string(key));
    return shared::Status{.message = status.ToString(), .ok = status.ok()};
  }

  shared::Status GEDSWrapper::delete_object_prefix(const rust::Str bucket, const rust::Str prefix) const {
    const auto status = gedsPtr->deleteObjectPrefix(std::string(bucket), std::string(prefix));
    return shared::Status{.message = status.ToString(), .ok = status.ok()};
  }

  shared::Status GEDSWrapper::rename(const rust::Str src_bucket, const rust::Str src_key,
                                     const rust::Str dest_bucket, const rust::Str dest_key) const {
    const auto status = gedsPtr->rename(std::string(src_bucket), std::string(src_key), std::string(dest_bucket),
                                        std::string(dest_key));
    return shared::Status{.message = status.ToString(), .ok = status.ok()};
  }

  shared::Status GEDSWrapper::rename_prefix(const rust::Str src_bucket, const rust::Str src_prefix,
                                            const rust::Str dest_bucket, const rust::Str dest_prefix) const {
    const auto status = gedsPtr->renamePrefix(std::string(src_bucket), std::string(src_prefix),
                                              std::string(dest_bucket), std::string(dest_prefix));
    return shared::Status{.message = status.ToString(), .ok = status.ok()};
  }

  shared::Status GEDSWrapper::copy(const rust::Str src_bucket, const rust::Str src_key,
                                   const rust::Str dest_bucket, const rust::Str dest_key) const {
    const auto status = gedsPtr->copy(std::string(src_bucket), std::string(src_key), std::string(dest_bucket),
                                      std::string(dest_key));
    return shared::Status{.message = status.ToString(), .ok = status.ok()};
  }

  shared::Status GEDSWrapper::copy_prefix(const rust::Str src_bucket, const rust::Str src_prefix,
                                          const rust::Str dest_bucket, const rust::Str dest_prefix) const {
    const auto status = gedsPtr->copyPrefix(std::string(src_bucket), std::string(src_prefix),
                                            std::string(dest_bucket), std::string(dest_prefix));
    return shared::Status{.message = status.ToString(), .ok = status.ok()};
  }

  rust::String GEDSWrapper::local_path(const rust::Str bucket, const rust::Str key) const {
    const auto path = gedsPtr->getLocalPath(std::string(bucket), std::string(key));
    return {path};
  }

  shared::Status GEDSWrapper::register_object_store_config(const rust::Str bucket,
                                                           const rust::Str endpoint_url,
                                                           const rust::Str access_key,
                                                           const rust::Str secret_key) const {
    const auto status = gedsPtr->registerObjectStoreConfig(std::string(bucket), std::string(endpoint_url),
                                                           std::string(access_key), std::string(secret_key));
    return shared::Status{.message = status.ToString(), .ok = status.ok()};
  }

  shared::Status GEDSWrapper::sync_object_store_configs() const {
    const auto status = gedsPtr->syncObjectStoreConfigs();
    return shared::Status{.message = status.ToString(), .ok = status.ok()};
  }

  void GEDSWrapper::relocate(const bool force) const {
    gedsPtr->relocate(force);
  }

  shared::Status GEDSWrapper::subscribe(const rust::Str bucket, const rust::Str key, const int &subscription_type) const {
  	geds::rpc::SubscriptionType type = static_cast<geds::rpc::SubscriptionType>(subscription_type);
    geds::SubscriptionEvent *event = new geds::SubscriptionEvent();
    event->bucket = std::string(bucket);
    event->key = std::string(key);
    event->subscriptionType = type;
    const auto status = gedsPtr->subscribe(*event);
    return shared::Status{.message = status.ToString(), .ok = status.ok()};
  }
}
