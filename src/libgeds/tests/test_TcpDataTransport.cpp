/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Logging.h"
#include "TcpDataTransport.h"

#include <cstdint>
#include <gtest/gtest.h>

using namespace geds::tcp_transport;

TEST(TcpDataTransport, Parsing) {
  auto request = createGetRequest("bucket", "key ", 0, 1073766400);

  std::string bucket;
  std::string key;
  size_t offset = SIZE_MAX;
  size_t length = SIZE_MAX;
  auto status = parseGetRequest(request, bucket, key, offset, length);
  if (!status.ok()) {
    LOG_ERROR(status.message());
  }
  ASSERT_TRUE(status.ok());

  ASSERT_EQ(bucket, "bucket");
  ASSERT_EQ(key, "key ");
  ASSERT_EQ(offset, 0);
  ASSERT_EQ(length, 1073766400);

  status = parseGetRequest("GET nase/baer\nRANGE 0 1073766400", bucket, key, offset, length);
  ASSERT_TRUE(status.ok());

  ASSERT_EQ(bucket, "nase");
  ASSERT_EQ(key, "baer");
  ASSERT_EQ(offset, 0);
  ASSERT_EQ(length, 1073766400);
}
