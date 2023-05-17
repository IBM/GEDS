/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <tuple>

#include <absl/status/status.h>
#include <boost/json.hpp>
#include <grpcpp/grpcpp.h>
#include <magic_enum.hpp>

#include "FormatISO8601.h"
#include "RWConcurrentObjectAdaptor.h"
#include "boost/json/detail/value_from.hpp"
#include "geds.grpc.pb.h"

enum class NodeState { Registered, Decommissioning, ReadyForShutdown, Unknown };

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
  NodeInformation(std::string uuid, std::string host, uint16_t port);
  NodeInformation(NodeInformation &) = delete;
  NodeInformation(NodeInformation &&) = delete;
  NodeInformation &operator=(NodeInformation &) = delete;
  NodeInformation &operator=(NodeInformation &&) = delete;

  absl::Status connect();
  absl::Status disconnect();

  // Subject to state mutex
  absl::Status queryHeartBeat();

  const std::string uuid;
  const std::string host;
  const uint16_t port;

  void setState(NodeState state);
  NodeState state() const;

  std::string gedsHostUri() const;

  void updateHeartBeat(const NodeHeartBeat &heartBeat);
  std::tuple<NodeHeartBeat, std::chrono::time_point<std::chrono::system_clock>>
  lastHeartBeat() const;
  std::chrono::time_point<std::chrono::system_clock> lastCheckin();
  // End subject to state mutex

  absl::Status downloadObjects(const std::vector<std::shared_ptr<RelocatableObject>> &objects);
  absl::Status purgeLocalObjects(const std::vector<std::shared_ptr<RelocatableObject>> &objects);
};

void tag_invoke(boost::json::value_from_tag, boost::json::value &jv,
                std::shared_ptr<NodeInformation> const &n);
