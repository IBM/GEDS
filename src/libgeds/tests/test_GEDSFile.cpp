/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#include "GEDS.h"

#include <climits>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Filesystem.h"
#include "GEDSLocalFileHandle.h"

TEST(GEDSFile, ReadWrite) {
  auto path = geds::filesystem::tempFile("test_GEDSFile");
  auto service_mock = std::shared_ptr<GEDS>(nullptr);
  auto handleStatus = GEDSLocalFileHandle::factory(service_mock, "test", "test", path);
  ASSERT_TRUE(handleStatus.ok());
  auto handle = handleStatus.value();
  ASSERT_TRUE(handle->seal().ok());

  auto file = handle->open().value();

  const auto data = std::string{"Hello World!"};
  auto length = data.size();
  auto writeStatus = file.write(data.c_str(), 0, length);
  ASSERT_TRUE(writeStatus.ok());

  auto buffer = std::vector<char>(length + 10, 0);
  auto readStatus = file.read(buffer, 0, length + 10);
  ASSERT_TRUE(readStatus.ok());
  ASSERT_EQ(readStatus.value(), length);
  ASSERT_EQ(std::string{data}, std::string{&buffer[0]});

  std::filesystem::remove(path);
}

template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
void readWriteTest() {
  auto path = geds::filesystem::tempFile("test_GEDSFile");
  auto service_mock = std::shared_ptr<GEDS>(nullptr);
  auto handleStatus = GEDSLocalFileHandle::factory(service_mock, "test", "test", path);
  ASSERT_TRUE(handleStatus.ok());
  auto handle = handleStatus.value();
  ASSERT_TRUE(handle->seal().ok());

  auto file = handle->open().value();

  auto data = std::vector<T>{(T)-1,
                             (T)-2,
                             0,
                             1,
                             2,
                             std::numeric_limits<T>::max(),
                             std::numeric_limits<T>::min(),
                             std::numeric_limits<T>::lowest(),
                             std::numeric_limits<T>::epsilon(),
                             std::numeric_limits<T>::round_error(),
                             std::numeric_limits<T>::infinity(),
                             std::numeric_limits<T>::min()};
  auto length = data.size();
  auto writeStatus = file.write(data, 0, 0, length);
  ASSERT_TRUE(writeStatus.ok());

  std::vector<T> resultBuffer;
  auto readStatus = file.read(resultBuffer, 0, 0, length + 1);
  ASSERT_TRUE(readStatus.ok());
  ASSERT_EQ(readStatus.value(), length);
  ASSERT_EQ(resultBuffer.size(), data.size());
  ASSERT_EQ(data, resultBuffer);

  std::filesystem::remove(path);
}

TEST(GEDSFile, ReadWrite_IntArray) {
  readWriteTest<char>();
  readWriteTest<uint8_t>();
  readWriteTest<int8_t>();
  readWriteTest<int16_t>();
  readWriteTest<uint16_t>();
  readWriteTest<int32_t>();
  readWriteTest<uint32_t>();
  readWriteTest<int64_t>();
  readWriteTest<uint64_t>();
  readWriteTest<float>();
  readWriteTest<double>();
  readWriteTest<long double>();
}
