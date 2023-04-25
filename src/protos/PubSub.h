/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GEDS_PUBSUB_H
#define GEDS_PUBSUB_H

#include <cstdint>
#include <string>

#include "geds.grpc.pb.h"

namespace geds {

struct SubscriptionEvent {
  std::string subscriber_id;
  std::string bucket;
  std::string key;
  geds::rpc::SubscriptionType subscriptionType;

  SubscriptionEvent(std::string bucket, std::string key,
                    geds::rpc::SubscriptionType subscriptionType)
      : bucket(std::move(bucket)), key(std::move(key)), subscriptionType(subscriptionType) {}

  SubscriptionEvent() {}

  bool operator==(const SubscriptionEvent &other) const {
    return subscriber_id == other.subscriber_id && bucket == other.bucket && key == other.key &&
           subscriptionType == other.subscriptionType;
  }
};

} // namespace geds
#endif // GEDS_PUBSUB_H
