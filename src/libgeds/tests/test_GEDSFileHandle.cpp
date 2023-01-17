/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Filesystem.h"
#include "GEDSFile.h"
#include "GEDSInternal.h"
#include "GEDSLocalFileHandle.h"
#include "GEDSService.h"

#include <cassert>
#include <gtest/gtest.h>
#include <memory>

TEST(GEDSFileHandle, openCount) {
  auto service_mock = std::shared_ptr<GEDS>(nullptr);
  auto path = geds::filesystem::tempFile("test_GEDSFileHandle");
  auto handleStatus = GEDSLocalFileHandle::factory(service_mock, "test", "test", path);
  ASSERT_TRUE(handleStatus.ok());
  auto handle = handleStatus.value();
  ASSERT_TRUE(handle->seal().ok());

  assert(handle->openCount() == 0);
  {
    auto f = handle->open();
    assert(handle->openCount() == 1);
    auto f2 = f;
    assert(handle->openCount() == 2);
    auto fMove = std::move(f);
    assert(handle->openCount() == 2);
  }
  assert(handle->openCount() == 0);
}

TEST(GEDSFileHandle, stringstream) {

  auto service_mock = std::shared_ptr<GEDS>(nullptr);
  auto path = geds::filesystem::tempFile("test_GEDSFileHandle");
  auto handleStatus = GEDSLocalFileHandle::factory(service_mock, "test", "test", path);
  ASSERT_TRUE(handleStatus.ok());
  auto handle = handleStatus.value();
}
