/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Nodes.h"

#include <algorithm>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "Logging.h"
#include "geds.grpc.pb.h"

absl::Status Nodes::registerNode(const std::string &identifier) {
  auto val = std::make_shared<NodeInformation>(identifier);
  auto existing = _nodes.insertOrExists(identifier, val);
  if (existing.get() != val.get()) {
    // auto diff = std::chrono::duration_cast<std::chrono::minutes>(now - existing->lastCheckin);
    if (existing->state() == NodeState::Decomissioning) {
      // Allow reregistering decomissioned nodes.
      auto connect = val->connect();
      if (!connect.ok()) {
        LOG_ERROR("Unable to establish backchannel to " + identifier +
                  " unable to decomission node!");
      }
      _nodes.insertOrReplace(identifier, val);

      return absl::OkStatus();
    }
    auto message = "Node " + identifier + " was already registered!";
    LOG_ERROR(message);
    return absl::AlreadyExistsError(message);
  }

  auto connect = val->connect();
  if (!connect.ok()) {
    LOG_ERROR("Unable to establish backchannel to " + identifier + " unable to decomission node!");
  }
  return absl::OkStatus();
}

absl::Status Nodes::unregisterNode(const std::string &identifier) {
  auto removed = _nodes.getAndRemove(identifier);
  if (!removed.value()) {
    auto message = "Unable to remove " + identifier + " not found!";
    LOG_ERROR(message);
    return absl::NotFoundError(message);
  }
  return absl::OkStatus();
}

absl::Status Nodes::heartbeat(const std::string &identifier, const NodeHeartBeat &heartbeat) {
  auto val = _nodes.get(identifier);
  if (!val.value()) {
    auto message = "Unable to process heartbeat " + identifier + " not found!";
    LOG_ERROR(message);
    return absl::NotFoundError(message);
  }
  (*val)->updateHeartBeat(heartbeat);
  return absl::OkStatus();
}

absl::Status Nodes::decomissionNodes(const std::vector<std::string> &nodes,
                                     std::shared_ptr<MDSKVS> kvs) {
  if (!_isDecommissioning.try_lock()) {
    return absl::UnavailableError("Already decomissioning.");
  }

  for (const auto &node : nodes) {
    auto existing = _nodes.get(node);
    if (!existing.has_value()) {
      LOG_ERROR("Unable to decomission: Node " + node + " since it does not exist!");
      continue;
    }
    (*existing)->setState(NodeState::Decomissioning);
  }

  // Prefix nodes with geds://
  std::vector<std::string> prefixedNodes(nodes.size());
  for (const auto &node : nodes) {
    prefixedNodes.emplace_back("geds://" + node);
  }

  // Find all buckets.
  auto buckets = kvs->listBuckets();
  if (!buckets.ok()) {
    LOG_ERROR("Unable to list buckets when decomissioning: ", buckets.status().message());
    _isDecommissioning.unlock();
    return buckets.status();
  }

  // Find all objects that need to be relocated.
  std::vector<std::shared_ptr<RelocatableObject>> objects;
  for (const auto &bucketName : *buckets) {
    auto bucket = kvs->getBucket(bucketName);
    if (!bucket.ok()) {
      continue;
    }
    (*bucket)->forall([&objects, &prefixedNodes, &bucketName](const utility::Path &path,
                                                              const geds::ObjectInfo &obj) {
      // Do not relocate cached blocks.
      if (path.name.starts_with("_$cachedblock$/")) {
        return;
      }
      for (const auto &n : prefixedNodes) {
        if (obj.location.starts_with(n)) {
          objects.emplace_back(
              new RelocatableObject{.bucket = bucketName, .key = path.name, .size = obj.size});
        }
      }
    });
  }

  std::sort(objects.begin(), objects.end(),
            [](const std::shared_ptr<RelocatableObject> &a,
               const std::shared_ptr<RelocatableObject> &b) { return a->size > b->size; });

  // List all available nodes.
  struct NodeTargetInfo {
    std::shared_ptr<NodeInformation> node;
    std::vector<std::shared_ptr<RelocatableObject>> objects;
    size_t available;
    size_t target;
  };
  std::vector<std::shared_ptr<NodeTargetInfo>> targetNodes;
  _nodes.forall([&targetNodes](std::shared_ptr<NodeInformation> &info) {
    if (info->state() == NodeState::Registered) {
      const auto [hb, _] = info->lastHeartBeat();

      size_t storageAvailable =
          (hb.storageUsed > hb.storageAllocated) ? 0 : hb.storageAllocated - hb.storageUsed;
      targetNodes.emplace_back(new NodeTargetInfo{
          .node = info, .objects = {}, .available = storageAvailable, .target = 0});
    }
  });
  if (targetNodes.size() == 0) {
    return absl::UnavailableError("No target nodes available!");
  }

  //  Sort target nodes based on available size.
  auto targetNodeCompare = [](const std::shared_ptr<NodeTargetInfo> &a,
                              const std::shared_ptr<NodeTargetInfo> &b) {
    auto aAvail = (a->available > a->target) ? a->available - a->target : 0;
    auto bAvail = (b->available > b->target) ? b->available - b->target : 0;
    return aAvail > bAvail || a->available > b->available;
  };
  std::sort(targetNodes.begin(), targetNodes.end(), targetNodeCompare);

  // Simple binpacking by filling up the empty nodes.
  std::vector<std::shared_ptr<RelocatableObject>> nonRelocatable;
  for (auto &obj : objects) {
    bool foundTarget = false;
    for (auto &target : targetNodes) {
      if (target->target + obj->size < target->available) {
        foundTarget = true;
        target->objects.push_back(obj);
        target->target += obj->size;
      }
    }
    if (!foundTarget) {
      LOG_ERROR("Unable to relocate ", obj->bucket, "/", obj->key, " (", obj->size, " bytes)");
      nonRelocatable.push_back(obj);
    }
  }

  std::vector<std::thread> threads;
  threads.reserve(targetNodes.size());
  for (auto &target : targetNodes) {
    threads.emplace_back([target] {
      auto status = target->node->downloadObjects(target->objects);
      if (!status.ok()) {
        LOG_ERROR("Unable to relocate objects to ", target->node->identifier);
      }
    });
  }
  for (auto &thread : threads) {
    thread.join();
  }

  _isDecommissioning.unlock();
  return absl::OkStatus();
}
