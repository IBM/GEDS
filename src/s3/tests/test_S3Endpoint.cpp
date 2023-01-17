/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <gtest/gtest.h>
#include <iostream>
#include <istream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "ObjectStoreConfig.h"
#include "S3Endpoint.h"

#ifndef S3_TEST_ENDPOINT
#define S3_TEST_ENDPOINT "http://localhost"
#endif
#ifndef S3_TEST_ACCESS_KEY
#define S3_TEST_ACCESS_KEY "test"
#endif
#ifndef S3_TEST_SECRET_KEY
#define S3_TEST_SECRET_KEY "test"
#endif

#define ADD_QUOTES_(x) #x
#define ADD_QUOTES(x) ADD_QUOTES_(x)

#define S3_TEST_ENDPOINT_ ADD_QUOTES(S3_TEST_ENDPOINT)
#define S3_TEST_ACCESS_KEY_ ADD_QUOTES(S3_TEST_ACCESS_KEY)
#define S3_TEST_SECRET_KEY_ ADD_QUOTES(S3_TEST_SECRET_KEY)

class S3EndpointTest : public ::testing::Test {
protected:
  geds::s3::Endpoint endpoint{S3_TEST_ENDPOINT_, S3_TEST_ACCESS_KEY_, S3_TEST_SECRET_KEY_};
  const std::string bucket = "geds-test";
};

TEST_F(S3EndpointTest, List) {
  auto ls = endpoint.list(bucket, "folder/", '/');
  if (!ls.ok()) {
    ls.value();
  }
  ASSERT_EQ(ls->size(), 12);
  for (const auto &el : *ls) {
    std::cout << "'" << el.key << "'";
    if (el.isDirectory) {
      std::cout << " (Directory)" << std::endl;
    } else {
      std::cout << ": " << el.size << " bytes" << std::endl;
    }
  }
}

TEST_F(S3EndpointTest, ListRecursive) {
  auto ls = endpoint.list(bucket, "/", '/');
  if (!ls.ok()) {
    ls.value();
  }
  for (const auto &el : *ls) {
    std::cout << "'" << el.key << "'";
    if (el.isDirectory) {
      std::cout << " (Directory)" << std::endl;
    } else {
      std::cout << ": " << el.size << " bytes" << std::endl;
    }
  }
}

TEST_F(S3EndpointTest, StatusFolder) {
  auto st = endpoint.folderStatus(bucket, "folder/", '/');
  if (!st.ok()) {
    st.value();
  }
  ASSERT_EQ(st->size, 0);
  ASSERT_EQ(st->isDirectory, true);
}

TEST_F(S3EndpointTest, StatusFile) {
  auto st = endpoint.fileStatus(bucket, "folder/0");
  if (!st.ok()) {
    st.value();
  }
  ASSERT_EQ(st->isDirectory, false);
}

TEST_F(S3EndpointTest, Read) {

  // Read entire string
  {
    auto buffer = std::vector<char>(100, 0);
    auto status = endpoint.readBytes(bucket, "testfile.txt",
                                     reinterpret_cast<uint8_t *>(&buffer[0]), 0, buffer.size());
    ASSERT_TRUE(status.ok());
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(*status, 15);
    auto message = std::string{&buffer[0]};
    ASSERT_EQ(message, "AHello World!A\n");
  }
  // Read partial string
  {
    auto buffer = std::vector<char>(100, 1);
    auto status =
        endpoint.readBytes(bucket, "testfile.bin", reinterpret_cast<uint8_t *>(&buffer[0]), 1, 12);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(*status, 12);
    buffer[*status + 1] = '\0';
    auto message = std::string{&buffer[0]};
    ASSERT_EQ(message, "Hello World!\x1");
  }
}

TEST_F(S3EndpointTest, ReadWrite) {
  const std::string_view message =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
      "tempor incididunt "
      "ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud "
      "exercitation "
      "ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute "
      "irure dolor in "
      "reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla "
      "pariatur. Excepteur "
      "sint occaecat cupidatat non proident, sunt in culpa qui officia "
      "deserunt mollit anim id est "
      "laborum.";

  auto testKey = "unittest/s3EndPointTest" + std::to_string(std::rand());
  std::cout << "Using " << bucket << "/" << testKey << " for temporary test data.";

  // Write string.
  {
    auto stream = std::make_shared<std::stringstream>();
    *stream << message;
    auto status = endpoint.putObject(bucket, testKey, stream);
    if (!status.ok()) {
      std::cerr << status.message() << std::endl;
    }
    ASSERT_TRUE(status.ok());
  }

  // Read string.
  {
    std::stringstream stream;
    auto status = endpoint.read(bucket, testKey, stream);
    if (!status.ok()) {
      std::cerr << status.status().message() << std::endl;
    }
    ASSERT_TRUE(status.ok());
    auto len = status.value();
    ASSERT_EQ(message.size(), len);
    ASSERT_EQ(std::string{message}, std::string{stream.str()});
  }

  // Delete object again.
  {
    auto status = endpoint.deleteObject(bucket, testKey);
    if (!status.ok()) {
      std::cerr << status.message() << std::endl;
    }
    ASSERT_TRUE(status.ok());
  }
}

TEST_F(S3EndpointTest, ReadWrite_Array) {
  const std::string_view message =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
      "tempor incididunt "
      "ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud "
      "exercitation "
      "ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute "
      "irure dolor in "
      "reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla "
      "pariatur. Excepteur "
      "sint occaecat cupidatat non proident, sunt in culpa qui officia "
      "deserunt mollit anim id est "
      "laborum.";

  auto testKey = "unittest/s3EndPointTest" + std::to_string(std::rand());
  std::cout << "Using " << bucket << "/" << testKey << " for temporary test data.";

  // Write string.
  {
    auto status = endpoint.putObject(
        bucket, testKey, reinterpret_cast<const uint8_t *>(message.data()), message.size());
    if (!status.ok()) {
      std::cerr << status.message() << std::endl;
    }
    ASSERT_TRUE(status.ok());
  }

  // Read string.
  {
    auto buffer = std::vector<char>(message.size() + 10, 0);
    std::stringstream stream;
    auto status = endpoint.readBytes(bucket, testKey, reinterpret_cast<uint8_t *>(&buffer[0]), 0,
                                     buffer.size());
    if (!status.ok()) {
      std::cerr << status.status().message() << std::endl;
    }
    ASSERT_TRUE(status.ok());
    auto len = status.value();
    ASSERT_EQ(message.size(), len);
    ASSERT_EQ(std::string{message}, std::string{&buffer[0]});
  }

  // Delete object again.
  {
    auto status = endpoint.deleteObject(bucket, testKey);
    if (!status.ok()) {
      std::cerr << status.message() << std::endl;
    }
    ASSERT_TRUE(status.ok());
  }
}
