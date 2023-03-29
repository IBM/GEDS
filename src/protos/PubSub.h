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
 std::string bucket;
 std::string key;
 std::string keyPrefix;
 geds::rpc::SubscriptionType subscriptionType;

 SubscriptionEvent(std::string bucket, std::string key, std::string keyPrefix,
                   geds::rpc::SubscriptionType subscriptionType)
     : bucket(std::move(bucket)), key(std::move(key)), keyPrefix(std::move(keyPrefix)),
       subscriptionType(subscriptionType) {}

 bool operator==(const SubscriptionEvent &other) const {
   return bucket == other.bucket && key == other.key && keyPrefix == other.keyPrefix &&
          subscriptionType == other.subscriptionType;
 }
};

} // namespace geds
#endif // GEDS_PUBSUB_H
