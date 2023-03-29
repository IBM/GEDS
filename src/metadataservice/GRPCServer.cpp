/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "GRPCServer.h"

#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <absl/status/status.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/server.h>
#include <grpcpp/support/status.h>

#include "FormatISO8601.h"
#include "Logging.h"
#include "ObjectStoreConfig.h"
#include "ObjectStoreHandler.h"
#include "ParseGRPC.h"
#include "Status.h"
#include "Version.h"

#include "geds.grpc.pb.h"
#include "geds.pb.h"

#define LOG_ACCESS(...)                                                                            \
  geds::logging::LogTimestamp(std::clog, context->peer(), ": ", __VA_ARGS__) // NOLINT

class MetadataServiceImpl final : public geds::rpc::MetadataService::Service {
  std::shared_ptr<KVS> _kvs;
  ObjectStoreHandler _objectStoreHandler;

public:
  MetadataServiceImpl(std::shared_ptr<KVS> kvs) : _kvs(kvs) {}

  geds::ObjectID convert(const ::geds::rpc::ObjectID *r) {
    return geds::ObjectID(r->bucket(), r->key());
  }

  geds::Object convert(const ::geds::rpc::Object *r) {
    return geds::Object{
        convert(&r->id()),
        geds::ObjectInfo{
            r->info().location(), r->info().size(), r->info().sealedoffset(),
            (r->info().has_metadata() ? std::make_optional(r->info().metadata()) : std::nullopt)}};
  }

protected:
  grpc::Status GetConnectionInformation(::grpc::ServerContext *context,
                                        const ::geds::rpc::EmptyParams * /* unused request */,
                                        ::geds::rpc::ConnectionInformation *response) override {

    LOG_ACCESS("get connection information");

    const auto result = geds::GetAddressFromGRPCPeer(context->peer());
    if (result.ok()) {
      response->set_remoteaddress(result.value());
    } else {
      auto error = response->mutable_error();
      convertStatus(error, result.status());
    }
    return grpc::Status::OK;
  }

  ::grpc::Status RegisterObjectStore(::grpc::ServerContext *context,
                                     const ::geds::rpc::ObjectStoreConfig *request,
                                     ::geds::rpc::StatusResponse *response) override {
    LOG_ACCESS("register object store ", request->endpointurl(), " for bucket ", request->bucket(),
               "'.");

    auto status = _objectStoreHandler.insertConfig(std::make_shared<geds::ObjectStoreConfig>(
        request->bucket(), request->endpointurl(), request->accesskey(), request->secretkey()));
    convertStatus(response, status);

    return grpc::Status::OK;
  }

  ::grpc::Status ListObjectStores(::grpc::ServerContext *context,
                                  const ::geds::rpc::EmptyParams * /* unused request */,
                                  ::geds::rpc::AvailableObjectStoreConfigs *response) override {
    LOG_ACCESS("list object stores");
    auto list = _objectStoreHandler.listConfigs();
    for (const auto &el : list) {
      auto mapping = response->add_mappings();
      mapping->set_bucket(el->bucket);
      mapping->set_endpointurl(el->endpointURL);
      mapping->set_accesskey(el->accessKey);
      mapping->set_secretkey(el->secretKey);
    }
    return grpc::Status::OK;
  }

  grpc::Status CreateBucket(::grpc::ServerContext *context, const ::geds::rpc::Bucket *request,
                            ::geds::rpc::StatusResponse *response) override {
    LOG_ACCESS("create bucket: ", request->bucket());
    auto result = _kvs->createBucket(request->bucket());
    convertStatus(response, result);
    return grpc::Status::OK;
  }

  grpc::Status DeleteBucket(::grpc::ServerContext *context, const ::geds::rpc::Bucket *request,
                            ::geds::rpc::StatusResponse *response) override {
    LOG_ACCESS("delete bucket: ", request->bucket());
    auto result = _kvs->deleteBucket(request->bucket());
    convertStatus(response, result);
    return grpc::Status::OK;
  }

  grpc::Status ListBuckets(::grpc::ServerContext *context,
                           const ::geds::rpc::EmptyParams * /* unused request */,
                           ::geds::rpc::BucketListResponse *response) override {
    LOG_ACCESS("List buckets");
    auto result = _kvs->listBuckets();
    if (result.ok()) {
      for (const auto &bucket : *result) {
        auto content = response->add_results();
        content->assign(bucket);
      }
    } else {
      auto error = response->mutable_error();
      convertStatus(error, result.status());
    }
    return grpc::Status::OK;
  }

  grpc::Status LookupBucket(::grpc::ServerContext *context, const ::geds::rpc::Bucket *request,
                            ::geds::rpc::StatusResponse *response) override {
    LOG_ACCESS("Lookup bucket: ", request->bucket());
    auto result = _kvs->bucketStatus(request->bucket());
    convertStatus(response, result);
    return grpc::Status::OK;
  }

  grpc::Status Create(::grpc::ServerContext *context, const ::geds::rpc::Object *request,
                      ::geds::rpc::StatusResponse *response) override {
    auto msg = !request->info().has_metadata()
                   ? "<none>"
                   : std::to_string(request->info().metadata().size()) + " bytes";

    LOG_ACCESS("create: ", request->id().bucket(), "/", request->id().key(), ": ",
               request->info().location(), " (", request->info().size(), ", ",
               request->info().sealedoffset(), ", ", msg, ")");
    auto result = _kvs->createObject(convert(request));
    convertStatus(response, result);
    return grpc::Status::OK;
  };

  grpc::Status Update(::grpc::ServerContext *context, const ::geds::rpc::Object *request,
                      ::geds::rpc::StatusResponse *response) override {
    auto msg = !request->info().has_metadata()
                   ? "<none>"
                   : std::to_string(request->info().metadata().size()) + " bytes";

    LOG_ACCESS("update: ", request->id().bucket(), "/", request->id().key(), ": ",
               request->info().location(), " (", request->info().size(), ", ",
               request->info().sealedoffset(), ", ", msg, ")");
    auto result = _kvs->updateObject(convert(request));
    convertStatus(response, result);
    return grpc::Status::OK;
  };

  grpc::Status Delete(::grpc::ServerContext *context, const ::geds::rpc::ObjectID *request,
                      ::geds::rpc::StatusResponse *response) override {
    LOG_ACCESS("delete: ", request->bucket(), "/", request->key());
    auto result = _kvs->deleteObject(convert(request));
    convertStatus(response, result);
    return grpc::Status::OK;
  };

  grpc::Status DeletePrefix(::grpc::ServerContext *context, const ::geds::rpc::ObjectID *request,
                            ::geds::rpc::StatusResponse *response) override {
    LOG_ACCESS("delete prefix: ", request->bucket(), "/", request->key());
    auto result = _kvs->deleteObjectPrefix(convert(request));
    convertStatus(response, result);
    return grpc::Status::OK;
  }

  grpc::Status Lookup(::grpc::ServerContext *context, const ::geds::rpc::ObjectID *request,
                      ::geds::rpc::ObjectResponse *response) override {
    LOG_ACCESS("lookup: ", request->bucket(), "/", request->key());
    auto status = _kvs->lookup(convert(request));
    if (status.ok()) {
      const auto &result = status.value();
      auto object = response->mutable_result();
      {
        auto objectId = object->mutable_id();
        objectId->set_bucket(result.id.bucket);
        objectId->set_key(result.id.key);
      }
      {
        auto objectInfo = object->mutable_info();
        objectInfo->set_location(result.info.location);
        objectInfo->set_size(result.info.size);
        objectInfo->set_sealedoffset(result.info.sealedOffset);
        if (result.info.metadata.has_value()) {
          objectInfo->set_metadata(*result.info.metadata);
        }
      }
    } else {
      auto error = response->mutable_error();
      convertStatus(error, status.status());
    }
    return grpc::Status::OK;
  }

  grpc::Status List(::grpc::ServerContext *context, const ::geds::rpc::ObjectListRequest *request,
                    ::geds::rpc::ObjectListResponse *response) override {
    LOG_ACCESS("list: ", request->prefix().bucket(), "/", request->prefix().key());
    char delimiter = request->has_delimiter() ? (char)request->delimiter() : 0;
    auto listing = _kvs->listObjects(convert(&request->prefix()), delimiter);
    if (listing.ok()) {
      for (const auto &result : listing->first) {
        auto object = response->add_results();
        auto objectId = object->mutable_id();
        objectId->set_bucket(result.id.bucket);
        objectId->set_key(result.id.key);
        auto objectInfo = object->mutable_info();
        objectInfo->set_location(result.info.location);
        objectInfo->set_size(result.info.size);
        objectInfo->set_sealedoffset(result.info.sealedOffset);
      }
      for (const auto &prefix : listing->second) {
        response->add_commonprefixes(prefix);
      }
    } else {
      auto error = response->mutable_error();
      convertStatus(error, listing.status());
    }
    return grpc::Status::OK;
  };
};

GRPCServer::GRPCServer(std::string serverAddress)
    : _kvs(std::make_shared<KVS>()), _grpcService(new MetadataServiceImpl(_kvs)),
      _serverAddress(std::move(serverAddress)) {}

GRPCServer::~GRPCServer() {}

absl::Status GRPCServer::startAndWait() {
  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();

  grpc::ServerBuilder builder;
  builder.AddListeningPort(_serverAddress, grpc::InsecureServerCredentials());
  builder.RegisterService(_grpcService.get());

  // TODO: FIXME Check if there is already an other service running on the same
  // port.
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::clog << "Metadata Server (" << utility::GEDSVersion() << ")" << std::endl;
  geds::logging::LogTimestamp(std::clog, "Server is listening on ", _serverAddress);
  server->Wait();
  return absl::OkStatus();
}
