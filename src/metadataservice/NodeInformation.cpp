/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "NodeInformation.h"

#include "Logging.h"
#include "Status.h"

NodeInformation::NodeInformation(std::string identifierArg)
    : identifier(std::move(identifierArg)) {}

absl::Status NodeInformation::connect() {
  auto channelLock = std::lock_guard<std::mutex>(_connectionMutex);
  auto lock = getWriteLock();
  if (_channel.get() != nullptr) {
    // Already connected.
    return absl::OkStatus();
  }
  try {
    _channel = grpc::CreateChannel(identifier, grpc::InsecureChannelCredentials());
    auto success =
        _channel->WaitForConnected(std::chrono::system_clock::now() + std::chrono::seconds(10));
    if (!success) {
      // Destroy channel.
      _channel = nullptr;
      return absl::UnavailableError("Could not connect to " + identifier + ".");
    }
    _stub = geds::rpc::GEDSService::NewStub(_channel);
  } catch (const std::exception &e) {
    _channel = nullptr;
    return absl::UnavailableError("Could not open channel with " + identifier +
                                  ". Reason: " + e.what());
  }
  return absl::OkStatus();
}

absl::Status NodeInformation::disconnect() {
  auto channelLock = std::lock_guard<std::mutex>(_connectionMutex);
  auto lock = getWriteLock();
  if (_channel.get() == nullptr) {
    // Already disconnected.
    return absl::OkStatus();
  }
  _stub.release();
  _channel = nullptr;
  return absl::OkStatus();
}

void NodeInformation::setState(NodeState state) {
  auto lock = getWriteLock();
  _state = state;
}
NodeState NodeInformation::state() {
  auto lock = getReadLock();
  return _state;
}

void NodeInformation::updateHeartBeat(const NodeHeartBeat &heartBeat) {
  auto lock = getWriteLock();
  _heartBeat = heartBeat;
  _lastCheckin = std::chrono::system_clock::now();
}

std::tuple<NodeHeartBeat, std::chrono::time_point<std::chrono::system_clock>>
NodeInformation::lastHeartBeat() {
  auto lock = getReadLock();
  return std::make_tuple(_heartBeat, _lastCheckin);
}

std::chrono::time_point<std::chrono::system_clock> NodeInformation::lastCheckin() {
  auto lock = getReadLock();
  return _lastCheckin;
}

absl::Status
NodeInformation::downloadObjects(const std::vector<std::shared_ptr<RelocatableObject>> &objects) {
  if (!_connectionMutex.try_lock()) {
    return absl::UnavailableError("Unable to pull objects: Lock is unavailable!");
  }

  geds::rpc::MultiObjectID request;
  geds::rpc::StatusResponse response;
  grpc::ClientContext context;

  for (const auto &o : objects) {
    auto obj = request.add_objects();
    obj->set_bucket(o->bucket);
    obj->set_key(o->key);
  }

  auto status = _stub->DownloadObjects(&context, request, &response);
  if (!status.ok()) {
    LOG_ERROR("Unable to execute object pull grpc call, status: ", status.error_code(), " ",
              status.error_details());
    return absl::UnknownError("Unable to execute object pull!");
  }

  return convertStatus(response);
}
