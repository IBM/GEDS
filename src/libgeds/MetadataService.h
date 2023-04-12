/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GEDS_METADATASERVICE_H
#define GEDS_METADATASERVICE_H

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <grpcpp/grpcpp.h>

#include "GEDSInternal.h"
#include "MDSKVS.h"
#include "Object.h"
#include "ObjectStoreConfig.h"
#include "PubSub.h"

#include "geds.grpc.pb.h"

namespace geds {

class MetadataService {
  ConnectionState _connectionState;
  MDSKVS _mdsCache;
  std::shared_ptr<grpc::Channel> _channel;
  std::unique_ptr<geds::rpc::MetadataService::Stub> _stub;
  std::string uuid;
  std::atomic<bool> subscribeStreamSingletonThreadFlag;
  std::atomic<bool> subscribeStreamContinueThreadFlag;

public:
  const std::string serverAddress;

  MetadataService() = delete;
  MetadataService(std::string serverAddress);

  ~MetadataService();

  absl::Status connect();

  absl::Status disconnect();

  absl::StatusOr<std::string> getConnectionInformation();

  absl::Status registerObjectStoreConfig(const ObjectStoreConfig &mapping);

  absl::StatusOr<std::vector<std::shared_ptr<ObjectStoreConfig>>> listObjectStoreConfigs();

  absl::Status createBucket(const std::string_view &bucket);

  absl::Status deleteBucket(const std::string_view &bucket);

  absl::StatusOr<std::vector<std::string>> listBuckets();

  absl::Status lookupBucket(const std::string_view &bucket);

  absl::Status createObject(const geds::Object &obj);

  absl::Status updateObject(const geds::Object &obj);

  absl::Status deleteObject(const geds::ObjectID &id);
  absl::Status deleteObject(const std::string &bucket, const std::string &key);

  absl::Status deleteObjectPrefix(const geds::ObjectID &id);
  absl::Status deleteObjectPrefix(const std::string &bucket, const std::string &key);

  absl::StatusOr<geds::Object> lookup(const geds::ObjectID &id, bool invalidate = false);
  absl::StatusOr<geds::Object> lookup(const std::string &bucket, const std::string &key,
                                      bool invalidate = false);

  /**
   * @brief List objects in `bucket` starting with `key` as prefix.
   */
  absl::StatusOr<std::vector<geds::Object>> listPrefix(const geds::ObjectID &id);
  absl::StatusOr<std::vector<geds::Object>> listPrefix(const std::string &bucket,
                                                       const std::string &keyPrefix);

  /**
   * @brief List objects in `bucket` starting with `key` as prefix. Objects that contain `delimiter`
   * in the postfix of the key are filtered. Delimiter `\0` is treated as no filter.
   */
  absl::StatusOr<std::pair<std::vector<geds::Object>, std::vector<std::string>>>
  listPrefix(const std::string &bucket, const std::string &keyPrefix, char delimiter);

  /**
   * @brief List objects from cache in `bucket` starting with `key` as prefix. Objects that contain
   * `delimiter` in the postfix of the key are filtered. Delimiter `\0` is treated as no filter.
   */
  absl::StatusOr<std::pair<std::vector<geds::Object>, std::vector<std::string>>>
  listPrefixFromCache(const std::string &bucket, const std::string &keyPrefix, char delimiter);

  /**
   * @brief Prefix search with `/` as delimiter.
   */
  absl::StatusOr<std::pair<std::vector<geds::Object>, std::vector<std::string>>>
  listFolder(const std::string &bucket, const std::string &keyPrefix);

  /**
   * @brief Create subscription stream for the subscriber. This has to be called in a thread.
   * However, possible memory leakage, as the thread will be running for ever until it is closed by
   * the MDS.
   */
  absl::Status subscribeStream(const geds::SubscriptionEvent &event);
  absl::Status setSubscribeStreamContinueAbortThreadFlag(bool threadFlag);
  /**
   * @brief Create subscription for bucket, objects and prefixes.
   */
  absl::Status subscribe(const geds::SubscriptionEvent &event);
  absl::Status unsubscribe(const geds::SubscriptionEvent &event);
};

} // namespace geds

#endif // GEDS_METADATASERVICE_H
