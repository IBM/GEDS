/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#include <gtest/gtest.h>

TEST(Test_GEDSS3FileHandle, ParseURL) {
  const std::string_view s3Prefix{"s3://"};
  const std::string url = "s3://bucket/path/1/2/3";

  ASSERT_TRUE(url.compare(0, s3Prefix.size(), s3Prefix) == 0);
  auto splitpos = url.find('/', s3Prefix.size());
  ASSERT_TRUE(splitpos != std::string::npos);
  auto bucketStr = url.substr(s3Prefix.size(), splitpos - s3Prefix.size());
  ASSERT_EQ(bucketStr, "bucket");
  auto pathStr = url.substr(splitpos + 1);
  ASSERT_EQ(pathStr, "path/1/2/3");
}
