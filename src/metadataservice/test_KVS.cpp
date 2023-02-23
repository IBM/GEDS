/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <absl/status/status.h>
#include <cstddef>
#include <gtest/gtest.h>

#include "MDSKVS.h"
#include "Object.h"

constexpr size_t NUM_ELEMENTS = 9;

TEST(KVS, Basic) {
  auto kvs = MDSKVS();

  auto bucket = "testBasic";
  EXPECT_EQ(kvs.createBucket(bucket).code(), absl::StatusCode::kOk);

  for (size_t i = 0; i <= NUM_ELEMENTS; i++) {
    EXPECT_EQ(kvs.createObject(
                     geds::Object{geds::ObjectID{bucket, "/" + std::to_string(i)},
                                  geds::ObjectInfo{"node" + std::to_string(i), 0, 0, std::nullopt}})
                  .code(),
              absl::StatusCode::kOk);
  }
  {
    auto list = kvs.listObjects(geds::ObjectID(bucket, "/"));
    EXPECT_TRUE(list.ok());
    EXPECT_EQ(list->first.size(), 10);
  }

  for (size_t i = 0; i <= NUM_ELEMENTS; i++) {
    EXPECT_EQ(kvs.createObject(
                     geds::Object{geds::ObjectID{bucket, "/2/" + std::to_string(i)},
                                  geds::ObjectInfo{"node" + std::to_string(i), 0, 0, std::nullopt}})
                  .code(),
              absl::StatusCode::kOk);
  }
  {
    auto list = kvs.listObjects(geds::ObjectID(bucket, "/2/"));
    EXPECT_TRUE(list.ok());
    EXPECT_EQ(list->first.size(), 10);
  }

  {
    auto list = kvs.listObjects(geds::ObjectID(bucket, "/"), '/');
    EXPECT_TRUE(list.ok());
    EXPECT_EQ(list->first.size(), 10);
  }
  {
    auto list = kvs.listObjects(geds::ObjectID(bucket, "/"));
    EXPECT_TRUE(list.ok());
    EXPECT_EQ(list->first.size(), 20);
  }
}

TEST(KVS, Delete) {
  auto kvs = MDSKVS();

  auto bucket = "testDelete";
  EXPECT_EQ(kvs.createBucket(bucket).code(), absl::StatusCode::kOk);

  for (size_t i = 0; i <= NUM_ELEMENTS; i++) {
    EXPECT_EQ(kvs.createObject(
                     geds::Object{geds::ObjectID{bucket, "/" + std::to_string(i)},
                                  geds::ObjectInfo{"node" + std::to_string(i), 0, 0, std::nullopt}})
                  .code(),
              absl::StatusCode::kOk);
  }

  EXPECT_EQ(kvs.deleteObject(geds::ObjectID{bucket, "/1"}).code(), absl::StatusCode::kOk);
  EXPECT_EQ(kvs.deleteObject(geds::ObjectID{bucket, "/1"}).code(), absl::StatusCode::kNotFound);
  EXPECT_EQ(kvs.deleteObjectPrefix(geds::ObjectID{bucket, "/"}).code(), absl::StatusCode::kOk);
  EXPECT_EQ(kvs.deleteObjectPrefix(geds::ObjectID{bucket, "/"}).code(),
            absl::StatusCode::kNotFound);
}
