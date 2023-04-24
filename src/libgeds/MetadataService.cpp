/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "MetadataService.h"

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <grpcpp/client_context.h>
#include <grpcpp/support/status.h>
#include <grpcpp/support/status_code_enum.h>
#include <optional>

#include "GEDS.h"
#include "Logging.h"
#include "ObjectStoreConfig.h"
#include "PubSub.h"
#include "Status.h"
#include "geds.grpc.pb.h"
#include "geds.pb.h"
#include "status.pb.h"

namespace geds {

static std::string printGRPCError(const grpc::Status &status) {
  if (status.error_message().size()) {
    return {status.error_message() + " code: " + std::to_string(status.error_code())};
  }
  return {"Code: " + std::to_string(status.error_code())};
}

#define METADATASERVICE_CHECK_CONNECTED                                                            \
  if (_connectionState != ConnectionState::Connected) {                                            \
    return absl::FailedPreconditionError("Not connected.");                                        \
  }

MetadataService::MetadataService(std::string serverAddress)
    : _connectionState(ConnectionState::Disconnected), _channel(nullptr),
      serverAddress(std::move(serverAddress)) {
  boost::uuids::uuid uuid_generated = boost::uuids::random_generator()();
  uuid = boost::lexical_cast<std::string>(uuid_generated);
}

MetadataService::~MetadataService() {
  if (_connectionState == ConnectionState::Connected) {
    disconnect().IgnoreError();
  }
}

absl::Status MetadataService::connect() {
  if (_connectionState != ConnectionState::Disconnected) {
    return absl::UnknownError("Cannot reinitialize service.");
  }
  try {
    assert(_channel.get() == nullptr);

    auto arguments = grpc::ChannelArguments();
    arguments.SetMaxReceiveMessageSize(64 * 1024 * 1024);

    _channel =
        grpc::CreateCustomChannel(serverAddress, grpc::InsecureChannelCredentials(), arguments);
    auto success = _channel->WaitForConnected(grpcDefaultDeadline());
    if (!success) {
      LOG_ERROR("Unable to connect to ", serverAddress);
      return absl::UnavailableError("Could not connect to " + serverAddress + ".");
    }
    _stub = geds::rpc::MetadataService::NewStub(_channel);
  } catch (std::exception &e) {
    auto msg = "Could not open channel with " + serverAddress + ". Reason" + std::string(e.what());
    LOG_ERROR(msg);
    return absl::UnavailableError(msg);
  }
  _connectionState = ConnectionState::Connected;
  // ToDO: Register client and implement stop().
  LOG_DEBUG("Connected to metadata service.");
  return absl::OkStatus();
}

absl::Status MetadataService::disconnect() {
  if (_connectionState != ConnectionState::Connected) {
    return absl::UnknownError("The service is in the wrong state.");
  }
  _channel = nullptr;
  _connectionState = ConnectionState::Disconnected;
  return absl::OkStatus();
}

absl::Status MetadataService::registerObjectStoreConfig(const ObjectStoreConfig &mapping) {
  METADATASERVICE_CHECK_CONNECTED;
  geds::rpc::ObjectStoreConfig request;
  geds::rpc::StatusResponse response;
  grpc::ClientContext context;

  request.set_bucket(mapping.bucket);
  request.set_endpointurl(mapping.endpointURL);
  request.set_accesskey(mapping.accessKey);
  request.set_secretkey(mapping.secretKey);

  auto status = _stub->RegisterObjectStore(&context, request, &response);
  if (!status.ok()) {
    return absl::UnavailableError("Unable to execute RegisterObjectStore: " +
                                  status.error_message());
  }
  return convertStatus(response);
}

absl::StatusOr<std::vector<std::shared_ptr<ObjectStoreConfig>>>
MetadataService::listObjectStoreConfigs() {
  METADATASERVICE_CHECK_CONNECTED;
  geds::rpc::EmptyParams request;
  geds::rpc::AvailableObjectStoreConfigs response;
  grpc::ClientContext context;
  auto status = _stub->ListObjectStores(&context, request, &response);
  if (!status.ok()) {
    return absl::UnavailableError("Unable to execute ListObjectStores: " + status.error_message());
  }
  auto mappings = response.mappings();
  std::vector<std::shared_ptr<ObjectStoreConfig>> result;
  for (auto &m : mappings) {
    result.emplace_back(std::make_shared<ObjectStoreConfig>(m.bucket(), m.endpointurl(),
                                                            m.accesskey(), m.secretkey()));
  }
  return result;
}

absl::StatusOr<std::string> MetadataService::getConnectionInformation() {
  METADATASERVICE_CHECK_CONNECTED;
  geds::rpc::EmptyParams request;
  geds::rpc::ConnectionInformation response;
  grpc::ClientContext context;
  auto status = _stub->GetConnectionInformation(&context, request, &response);
  if (!status.ok()) {
    return absl::UnavailableError("Unable to execute GetConnectionInformation: " +
                                  status.error_message());
  }
  if (response.has_error()) {
    return convertStatus(response.error());
  }
  return response.remoteaddress();
}

absl::Status MetadataService::createBucket(const std::string_view &bucket) {
  METADATASERVICE_CHECK_CONNECTED;
  geds::rpc::Bucket request;
  request.set_bucket(std::string{bucket});

  geds::rpc::StatusResponse response;
  grpc::ClientContext context;

  auto status = _stub->CreateBucket(&context, request, &response);
  if (!status.ok()) {
    return absl::UnavailableError("Unable to execute CreateBucket command: " +
                                  status.error_message());
  }
  return convertStatus(response);
}

absl::Status MetadataService::deleteBucket(const std::string_view &bucket) {
  METADATASERVICE_CHECK_CONNECTED;
  geds::rpc::Bucket request;
  request.set_bucket(std::string{bucket});

  geds::rpc::StatusResponse response;
  grpc::ClientContext context;

  auto status = _stub->DeleteBucket(&context, request, &response);
  if (!status.ok()) {
    return absl::UnavailableError("Unable to execute DeleteBucket command: " +
                                  status.error_message());
  }
  return convertStatus(response);
}

absl::StatusOr<std::vector<std::string>> MetadataService::listBuckets() {
  METADATASERVICE_CHECK_CONNECTED;
  geds::rpc::EmptyParams request;

  geds::rpc::BucketListResponse response;
  grpc::ClientContext context;

  auto status = _stub->ListBuckets(&context, request, &response);
  if (!status.ok()) {
    return absl::UnavailableError("Unable to execute ListBuckets command: " +
                                  status.error_message());
  }
  if (response.has_error()) {
    return convertStatus(response.error());
  }
  const auto &bucketList = response.results();

  std::vector<std::string> result;
  result.reserve(bucketList.size());
  for (const auto &r : bucketList) {
    result.emplace_back(r);
  }
  return result;
}

absl::Status MetadataService::lookupBucket(const std::string_view &bucket) {
  METADATASERVICE_CHECK_CONNECTED;

  geds::rpc::Bucket request;
  request.set_bucket(std::string{bucket});

  geds::rpc::StatusResponse response;
  grpc::ClientContext context;

  auto status = _stub->LookupBucket(&context, request, &response);
  if (!status.ok()) {
    return absl::UnavailableError("Unable to execute LookupBucket command: " +
                                  status.error_message());
  }
  auto s = convertStatus(response);
  if (!s.ok()) {
    (void)_mdsCache.deleteBucket(std::string{bucket});
  }
  return s;
}

absl::Status MetadataService::createObject(const geds::Object &obj) {
  METADATASERVICE_CHECK_CONNECTED;

  geds::rpc::Object request;
  auto id = request.mutable_id();
  id->set_bucket(obj.id.bucket);
  id->set_key(obj.id.key);
  auto info = request.mutable_info();
  info->set_location(obj.info.location);
  info->set_size(obj.info.size);
  info->set_sealedoffset(obj.info.sealedOffset);
  if (obj.info.metadata.has_value()) {
    info->set_metadata(obj.info.metadata.value());
  }

  geds::rpc::StatusResponse response;
  grpc::ClientContext context;

  auto status = _stub->Create(&context, request, &response);
  if (!status.ok()) {
    return absl::UnavailableError("Unable to execute Create command: " + status.error_message());
  }
  return convertStatus(response);
}

absl::Status MetadataService::updateObject(const geds::Object &obj) {
  METADATASERVICE_CHECK_CONNECTED;

  geds::rpc::Object request;
  auto id = request.mutable_id();
  id->set_bucket(obj.id.bucket);
  id->set_key(obj.id.key);
  auto info = request.mutable_info();
  info->set_location(obj.info.location);
  info->set_size(obj.info.size);
  info->set_sealedoffset(obj.info.sealedOffset);
  if (obj.info.metadata.has_value()) {
    info->set_metadata(obj.info.metadata.value());
  }
  geds::rpc::StatusResponse response;
  grpc::ClientContext context;

  auto status = _stub->Update(&context, request, &response);
  if (!status.ok()) {
    return absl::UnavailableError("Unable to execute Update command: " + printGRPCError(status));
  }
  return convertStatus(response);
}

absl::Status MetadataService::deleteObject(const geds::ObjectID &id) {
  METADATASERVICE_CHECK_CONNECTED;
  return deleteObject(id.bucket, id.key);
}

absl::Status MetadataService::deleteObject(const std::string &bucket, const std::string &key) {
  METADATASERVICE_CHECK_CONNECTED;

  (void)_mdsCache.deleteObject(bucket, key);

  geds::rpc::ObjectID request;
  request.set_bucket(bucket);
  request.set_key(key);

  geds::rpc::StatusResponse response;
  grpc::ClientContext context;

  auto status = _stub->Delete(&context, request, &response);
  if (!status.ok()) {
    return absl::UnavailableError("Unable to execute Delete command: " + printGRPCError(status));
  }
  return convertStatus(response);
}

absl::Status MetadataService::deleteObjectPrefix(const geds::ObjectID &id) {
  return deleteObjectPrefix(id.bucket, id.key);
}
absl::Status MetadataService::deleteObjectPrefix(const std::string &bucket,
                                                 const std::string &key) {
  METADATASERVICE_CHECK_CONNECTED;

  (void)_mdsCache.deleteObjectPrefix(bucket, key);

  geds::rpc::ObjectID request;
  request.set_bucket(bucket);
  request.set_key(key);

  geds::rpc::StatusResponse response;
  grpc::ClientContext context;

  auto status = _stub->DeletePrefix(&context, request, &response);
  if (!status.ok()) {
    return absl::UnavailableError("Unable to execute Delete command: " + printGRPCError(status));
  }
  return convertStatus(response);
}

absl::StatusOr<geds::Object> MetadataService::lookup(const geds::ObjectID &id, bool force) {
  return lookup(id.bucket, id.key, force);
}
absl::StatusOr<geds::Object> MetadataService::lookup(const std::string &bucket,
                                                     const std::string &key, bool invalidate) {
  METADATASERVICE_CHECK_CONNECTED;

  if (!invalidate) {
    LOG_DEBUG("Lookup cache", bucket, "/", key);
    auto c = _mdsCache.lookup(bucket, key);
    if (c.ok()) {
      return c;
    }
  }

  geds::rpc::ObjectID request;
  request.set_bucket(bucket);
  request.set_key(key);

  geds::rpc::ObjectResponse response;
  grpc::ClientContext context;

  LOG_DEBUG("Lookup remote", bucket, "/", key);

  auto status = _stub->Lookup(&context, request, &response);
  if (!status.ok()) {
    return absl::UnavailableError("Unable to execute Lookup command: " + printGRPCError(status));
  }
  if (response.has_error()) {
    return convertStatus(response.error());
  }
  const auto &r = response.result();
  auto obj_id = geds::ObjectID{r.id().bucket(), r.id().key()};
  auto obj_info = geds::ObjectInfo{
      r.info().location(), r.info().size(), r.info().sealedoffset(),
      (r.info().has_metadata() ? std::make_optional(r.info().metadata()) : std::nullopt)};

  auto result = geds::Object{obj_id, obj_info};
  (void)_mdsCache.createObject(result, true);
  return result;
}

absl::StatusOr<std::vector<geds::Object>> MetadataService::listPrefix(const geds::ObjectID &id) {
  return listPrefix(id.bucket, id.key);
}

absl::StatusOr<std::vector<geds::Object>>
MetadataService::listPrefix(const std::string &bucket, const std::string &keyPrefix) {
  auto result = listPrefix(bucket, keyPrefix, 0);
  if (result.ok()) {
    return result->first;
  }
  return result.status();
}

absl::StatusOr<std::pair<std::vector<geds::Object>, std::vector<std::string>>>
MetadataService::listPrefix(const std::string &bucket, const std::string &keyPrefix,
                            char delimiter) {
  METADATASERVICE_CHECK_CONNECTED;

  geds::rpc::ObjectListRequest request;
  auto prefix = request.mutable_prefix();
  prefix->set_bucket(bucket);
  prefix->set_key(keyPrefix);
  if (delimiter > 0) {
    request.set_delimiter(delimiter);
  }

  geds::rpc::ObjectListResponse response;
  grpc::ClientContext context;

  auto status = _stub->List(&context, request, &response);
  if (!status.ok()) {
    return absl::UnavailableError("Unable to execute List command: " + printGRPCError(status));
  }
  if (response.has_error()) {
    return convertStatus(response.error());
  }

  const auto &rpc_results = response.results();
  auto objects = std::vector<geds::Object>{};
  objects.reserve(rpc_results.size());
  for (auto i : rpc_results) {
    auto obj_id = geds::ObjectID{i.id().bucket(), i.id().key()};
    auto obj_info = geds::ObjectInfo{
        i.info().location(), i.info().size(), i.info().sealedoffset(),
        i.info().has_metadata() ? std::make_optional(i.info().metadata()) : std::nullopt};
    auto obj = geds::Object{obj_id, obj_info};
    (void)_mdsCache.createObject(obj, true);
    objects.emplace_back(std::move(obj));
  }
  return std::make_pair(objects, std::vector<std::string>{response.commonprefixes().begin(),
                                                          response.commonprefixes().end()});
}

absl::StatusOr<std::pair<std::vector<geds::Object>, std::vector<std::string>>>
MetadataService::listPrefixFromCache(const std::string &bucket, const std::string &keyPrefix,
                                     char delimiter) {

  auto status_or_objects = _mdsCache.listObjects(bucket, keyPrefix, delimiter);
  if (!status_or_objects.ok()) {
    status_or_objects = listPrefix(bucket, keyPrefix, delimiter);
  }
  return status_or_objects;
}

absl::StatusOr<std::pair<std::vector<geds::Object>, std::vector<std::string>>>
MetadataService::listFolder(const std::string &bucket, const std::string &keyPrefix) {
  return listPrefix(bucket, keyPrefix, Default_GEDSFolderDelimiter);
}

absl::Status MetadataService::subscribeStream(const geds::SubscriptionEvent &event) {
  METADATASERVICE_CHECK_CONNECTED;

  if (subscribeStreamSingletonThreadFlag) {
    return absl::OkStatus();
  }
  subscribeStreamSingletonThreadFlag = true;

  geds::rpc::SubscriptionStreamEvent subscription_stream_event;
  geds::rpc::SubscriptionStreamResponse subscription_response;
  grpc::ClientContext context;

  if (event.subscriber_id.empty()) {
    subscription_stream_event.set_subscriberid(uuid);
  } else {
    subscription_stream_event.set_subscriberid(event.subscriber_id);
  }

  std::unique_ptr<grpc::ClientReader<geds::rpc::SubscriptionStreamResponse>> reader(
      _stub->SubscribeStream(&context, subscription_stream_event));

  while (reader->Read(&subscription_response) && subscribeStreamContinueThreadFlag) {

    const auto &objectPublication = subscription_response.object();
    auto obj_id = geds::ObjectID{objectPublication.id().bucket(), objectPublication.id().key()};
    auto obj_info =
        geds::ObjectInfo{objectPublication.info().location(), objectPublication.info().size(),
                         objectPublication.info().sealedoffset()};
    auto obj = geds::Object{obj_id, obj_info};

    if (subscription_response.publicationtype() == geds::rpc::CREATE_OBJECT) {
      (void)_mdsCache.createObject(obj, true);
    } else if (subscription_response.publicationtype() == geds::rpc::UPDATE_OBJECT) {
      (void)_mdsCache.updateObject(obj);
    } else if (subscription_response.publicationtype() == geds::rpc::DELETE_OBJECT) {
      (void)_mdsCache.deleteObject(obj.id.bucket, obj.id.key);
    }

    LOG_DEBUG("Received subscription and added to cache (bucket, key): ", obj.id.bucket, " , ",
              obj.id.key);
  }
  subscribeStreamSingletonThreadFlag = false;
  auto status = reader->Finish();
  if (!status.ok()) {
    return absl::InternalError(status.error_message());
  }
  if (subscribeStreamContinueThreadFlag) {
    return subscribeStream(event);
  }
  return absl::OkStatus();
}

absl::Status MetadataService::setSubscribeStreamContinueAbortThreadFlag(bool threadFlag) {
  subscribeStreamContinueThreadFlag = threadFlag;
  return absl::OkStatus();
}

absl::Status MetadataService::subscribe(const geds::SubscriptionEvent &event) {
  METADATASERVICE_CHECK_CONNECTED

  geds::rpc::SubscriptionEvent subscription_event;
  geds::rpc::StatusResponse response;
  grpc::ClientContext context;

  if (event.subscriber_id.empty()) {
    subscription_event.set_subscriberid(uuid);
  } else {
    subscription_event.set_subscriberid(event.subscriber_id);
  }
  subscription_event.set_bucketid(std::string{event.bucket});
  subscription_event.set_key(std::string{event.key});
  subscription_event.set_subscriptiontype(event.subscriptionType);

  auto status = _stub->Subscribe(&context, subscription_event, &response);
  if (!status.ok()) {
    return absl::UnavailableError("Unable to execute CreateBucket command: " +
                                  status.error_message());
  }
  return convertStatus(response);
}

absl::Status MetadataService::unsubscribe(const geds::SubscriptionEvent &event) {
  METADATASERVICE_CHECK_CONNECTED;

  geds::rpc::SubscriptionEvent subscription_event;
  geds::rpc::StatusResponse response;
  grpc::ClientContext context;

  if (event.subscriber_id.empty()) {
    subscription_event.set_subscriberid(uuid);
  } else {
    subscription_event.set_subscriberid(event.subscriber_id);
  }
  subscription_event.set_bucketid(std::string{event.bucket});
  subscription_event.set_key(std::string{event.key});
  subscription_event.set_subscriptiontype(event.subscriptionType);

  auto status = _stub->Unsubscribe(&context, subscription_event, &response);
  if (!status.ok()) {
    return absl::UnavailableError("Unable to unsubscribe: " + status.error_message());
  }
  return convertStatus(response);
}

} // namespace geds