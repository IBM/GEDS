/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>

#include <absl/status/status.h>
#include <grpcpp/grpcpp.h>
#include <tuple>

#include "RWConcurrentObjectAdaptor.h"
#include "geds.grpc.pb.h"

enum class NodeState { Registered, Decomissioning, Unknown };

struct NodeHeartBeat {
  size_t memoryAllocated{0};
  size_t memoryUsed{0};
  size_t storageAllocated{0};
  size_t storageUsed{0};
};

struct RelocatableObject {
  const std::string &bucket;
  std::string key;
  size_t size;
};

class NodeInformation : public utility::RWConcurrentObjectAdaptor {
  std::mutex _connectionMutex;
  std::shared_ptr<grpc::Channel> _channel{nullptr};
  std::unique_ptr<geds::rpc::GEDSService::Stub> _stub{nullptr};

  // Subject to state mutex
  NodeState _state{NodeState::Unknown};
  NodeHeartBeat _heartBeat;
  std::chrono::time_point<std::chrono::system_clock> _lastCheckin;
  // End subject to state mutex
public:
  NodeInformation(std::string identifier);
  NodeInformation(NodeInformation &) = delete;
  NodeInformation(NodeInformation &&) = delete;
  NodeInformation &operator=(NodeInformation &) = delete;
  NodeInformation &operator=(NodeInformation &&) = delete;

  absl::Status connect();
  absl::Status disconnect();

  // Subject to state mutex
  absl::Status queryHeartBeat();
  const std::string identifier;

  void setState(NodeState state);
  NodeState state();

  void updateHeartBeat(const NodeHeartBeat &heartBeat);
  std::tuple<NodeHeartBeat, std::chrono::time_point<std::chrono::system_clock>> lastHeartBeat();
  std::chrono::time_point<std::chrono::system_clock> lastCheckin();
  // End subject to state mutex

  absl::Status downloadObjects(const std::vector<std::shared_ptr<RelocatableObject>> &objects);
};
