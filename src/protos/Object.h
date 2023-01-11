/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#ifndef GEDS_OBJECT_H
#define GEDS_OBJECT_H

#include <cstdint>
#include <string>

namespace geds {
struct ObjectInfo {
  std::string location;
  uint64_t size;
  uint64_t sealedOffset;

  bool operator==(const ObjectInfo &other) const {
    return location == other.location && size == other.size && sealedOffset == other.sealedOffset;
  }
};

struct ObjectID {
  std::string bucket;
  std::string key;

  ObjectID(std::string bucket, std::string key) : bucket(std::move(bucket)), key(std::move(key)) {}

  bool operator==(const ObjectID &other) const {
    return bucket == other.bucket && key == other.key;
  }
};

struct Object {
  ObjectID id;
  ObjectInfo info;

  bool operator==(const Object &other) const { return id == other.id && info == other.info; }
};

} // namespace geds
#endif // GEDS_OBJECT_H
