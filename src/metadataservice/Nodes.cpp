/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Nodes.h"

#include <algorithm>
#include <boost/json/object.hpp>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <boost/json/array.hpp>
#include <boost/json/serialize.hpp>
#include <boost/json/value_from.hpp>

#include "Logging.h"
#include "NodeInformation.h"
#include "absl/status/status.h"
#include "geds.grpc.pb.h"

absl::Status Nodes::registerNode(const std::string &uuid, const std::string &host, uint16_t port) {
  auto val = std::make_shared<NodeInformation>(uuid, host, port);
  auto existing = _nodes.insertOrExists(uuid, val);
  if (existing.get() != val.get()) {
    // auto diff = std::chrono::duration_cast<std::chrono::minutes>(now - existing->lastCheckin);
    if (existing->state() == NodeState::Decommissioning) {
      // Allow reregistering decommissioned nodes.
      auto connect = val->connect();
      if (!connect.ok()) {
        LOG_ERROR(connect.message());
      }
      _nodes.insertOrReplace(uuid, val);

      return absl::OkStatus();
    }
    auto message = "Node " + uuid + " was already registered!";
    LOG_ERROR(message);
    return absl::AlreadyExistsError(message);
  }

  auto connect = val->connect();
  if (connect.ok()) {
    val->setState(NodeState::Registered);
  } else {
    LOG_ERROR(connect.message());
  }
  return absl::OkStatus();
}

absl::Status Nodes::reregisterNode(const std::string &uuid) {
  auto exists = _nodes.get(uuid);
  if (!exists.has_value()) {
    return absl::NotFoundError("Node " + uuid + " does not exist!");
  }
  (*exists)->setState(NodeState::Registered);
  return absl::OkStatus();
}

absl::Status Nodes::unregisterNode(const std::string &uuid) {
  auto removed = _nodes.getAndRemove(uuid);
  if (!removed.value()) {
    auto message = "Unable to remove " + uuid + " not found!";
    LOG_ERROR(message);
    return absl::NotFoundError(message);
  }
  return absl::OkStatus();
}

absl::Status Nodes::heartbeat(const std::string &uuid, const NodeHeartBeat &heartbeat) {
  auto val = _nodes.get(uuid);
  if (!val.value()) {
    auto message = "Unable to process heartbeat " + uuid + " not found!";
    LOG_ERROR(message);
    return absl::NotFoundError(message);
  }
  (*val)->updateHeartBeat(heartbeat);
  return absl::OkStatus();
}

absl::Status Nodes::decommissionNodes(const std::vector<std::string> &nodes,
                                      std::shared_ptr<MDSKVS> kvs) {
  auto lock = std::unique_lock<std::mutex>(_isDecommissioning, std::try_to_lock);
  if (!lock.owns_lock()) {
    return absl::UnavailableError("Already decommissioning.");
  }

  // Mark hosts as decommissioning and determine host uris to collect objects.
  std::vector<std::string> gedsHostUris;
  std::vector<std::shared_ptr<NodeInformation>> hostsToDecommision;
  for (const auto &node : nodes) {
    auto existing = _nodes.get(node);
    if (!existing.has_value()) {
      return absl::UnavailableError("Unable to decommission node: " + node +
                                    " since it does not exist!");
    }
    (*existing)->setState(NodeState::Decommissioning);
    hostsToDecommision.emplace_back(*existing);
    gedsHostUris.emplace_back((*existing)->gedsHostUri());
  }

  // Find all buckets.
  auto buckets = kvs->listBuckets();
  if (!buckets.ok()) {
    LOG_ERROR("Unable to list buckets when decommissioning: ", buckets.status().message());
    return buckets.status();
  }

  // Find all objects that need to be relocated.
  std::vector<std::shared_ptr<RelocatableObject>> objects;
  for (const auto &bucketName : *buckets) {
    auto bucket = kvs->getBucket(bucketName);
    if (!bucket.ok()) {
      continue;
    }
    (*bucket)->forall([&objects, &gedsHostUris, &bucketName](const utility::Path &path,
                                                             const geds::ObjectInfo &obj) {
      // Do not relocate cached blocks.
      if (path.name.starts_with("_$cachedblock$/")) {
        return;
      }
      for (const auto &uri : gedsHostUris) {
        if (obj.location == uri) {
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
        break;
      }
    }
    if (!foundTarget) {
      LOG_ERROR("Unable to relocate ", obj->bucket, "/", obj->key, " (", obj->size, " bytes)");
      nonRelocatable.push_back(obj);
    }
  }

  std::atomic<size_t> failures;
  std::vector<std::thread> threads;
  threads.reserve(targetNodes.size());
  for (auto &target : targetNodes) {
    threads.emplace_back([target, &failures] {
      auto status = target->node->downloadObjects(target->objects);
      if (!status.ok()) {
        failures += 1;
        LOG_ERROR("Unable to relocate objects to ", target->node->host,
                  " uuid: ", target->node->uuid);
        return;
      }
    });
  }
  for (auto &thread : threads) {
    thread.join();
  }

  if (failures == 0) {
    for (auto &host : hostsToDecommision) {
      host->setState(NodeState::ReadyForShutdown);
    }
  }

  return absl::OkStatus();
}

void tag_invoke(boost::json::value_from_tag, boost::json::value &jv, Nodes const &n) {
  boost::json::array nv;
  const auto &info = n.information();
  info.forall([&nv](const std::shared_ptr<NodeInformation> &node) {
    nv.emplace_back(boost::json::value_from(node));
  });
  jv = {{"nodes", nv}};
}

void tag_invoke(boost::json::value_from_tag t, boost::json::value &jv,
                std::shared_ptr<Nodes> const &n) {
  tag_invoke(t, jv, *n);
}
