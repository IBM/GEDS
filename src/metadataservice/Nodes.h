/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

#include <absl/status/status.h>

#include "ConcurrentMap.h"
#include "MDSKVS.h"
#include "NodeInformation.h"
#include "geds.grpc.pb.h"

class Nodes {
  mutable std::shared_mutex _mutex;

  utility::ConcurrentMap<std::string, std::shared_ptr<NodeInformation>> _nodes;

  std::mutex _isDecommissioning;

public:
  Nodes() = default;
  Nodes(Nodes &) = delete;
  Nodes(Nodes &&) = delete;
  Nodes &operator=(Nodes &) = delete;
  Nodes &operator=(Nodes &&) = delete;
  ~Nodes() = default;

  const utility::ConcurrentMap<std::string, std::shared_ptr<NodeInformation>> &information() const {
    return _nodes;
  };

  absl::Status registerNode(const std::string &identifier);
  absl::Status unregisterNode(const std::string &identifier);
  absl::Status heartbeat(const std::string &identifier, const NodeHeartBeat &heartbeat);
  absl::Status decomissionNodes(const std::vector<std::string> &nodes, std::shared_ptr<MDSKVS> kvs);
};
