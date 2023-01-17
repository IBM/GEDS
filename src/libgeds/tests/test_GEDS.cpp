/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "GEDS.h"

#include <gtest/gtest.h>

TEST(GEDS, Basic) {}

TEST(GEDS, BucketName) {
  ASSERT_TRUE(GEDS::isValidBucketName("abc").ok());
  ASSERT_TRUE(GEDS::isValidBucketName("a-bc").ok());
  ASSERT_FALSE(GEDS::isValidBucketName("---").ok());

  ASSERT_FALSE(GEDS::isValidBucketName("xn--").ok());
  ASSERT_FALSE(GEDS::isValidBucketName("xn--bla").ok());
  ASSERT_TRUE(GEDS::isValidBucketName("axn--bla").ok());
  ASSERT_TRUE(GEDS::isValidBucketName("yxn--bla").ok());

  ASSERT_FALSE(GEDS::isValidBucketName("ABC").ok());
  ASSERT_FALSE(GEDS::isValidBucketName("a.B.c").ok());

  ASSERT_FALSE(GEDS::isValidBucketName("a/b").ok());
}

TEST(GEDS, KeyName) {
  ASSERT_FALSE(GEDS::isValidKeyName("").ok());
  ASSERT_FALSE(GEDS::isValidKeyName("./").ok());
  ASSERT_FALSE(GEDS::isValidKeyName("../").ok());
  ASSERT_FALSE(GEDS::isValidKeyName("/").ok());
  ASSERT_FALSE(GEDS::isValidKeyName("a/../").ok());

  ASSERT_TRUE(GEDS::isValidKeyName("com.ibm/hello-wörld/😃").ok());
}
