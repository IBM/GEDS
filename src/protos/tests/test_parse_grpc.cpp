/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "ParseGRPC.h"

TEST(ParseGRPC, IPv6) {
  ASSERT_EQ(geds::GetAddressFromGRPCPeer("ipv6:[2620:fe::fe]:53").value(), "[2620:fe::fe]");
  ASSERT_EQ(geds::GetAddressFromGRPCPeer("ipv6:[::]:443").value(), "[::]");
  ASSERT_EQ(geds::GetAddressFromGRPCPeer("ipv6:[2620:fe::fe]").value(), "[2620:fe::fe]");
  ASSERT_EQ(geds::GetAddressFromGRPCPeer("ipv6:[::]").value(), "[::]");
  ASSERT_FALSE(geds::GetAddressFromGRPCPeer("ipv6:[2620:fe::9],[2620:fe::fe]").ok());
  ASSERT_FALSE(geds::GetAddressFromGRPCPeer("ipv6:[2620:fe::9]:53,[2620:fe::fe]:53").ok());
}

TEST(ParseGRPC, IPv4) {
  ASSERT_EQ(geds::GetAddressFromGRPCPeer("ipv4:127.0.0.1").value(), "127.0.0.1");
  ASSERT_EQ(geds::GetAddressFromGRPCPeer("ipv4:127.0.0.1:1234").value(), "127.0.0.1");
  ASSERT_EQ(geds::GetAddressFromGRPCPeer("ipv4:www.ibm.com").value(), "www.ibm.com");
  ASSERT_FALSE(geds::GetAddressFromGRPCPeer("ipv4:149.112.112.112,9.9.9.9").ok());
  ASSERT_FALSE(geds::GetAddressFromGRPCPeer("ipv4:149.112.112.112:53,9.9.9.9:53").ok());
}

TEST(ParseGRPC, Invalid) {
  ASSERT_FALSE(geds::GetAddressFromGRPCPeer("149.112.112.112,9.9.9.9").ok());
  ASSERT_FALSE(geds::GetAddressFromGRPCPeer("ipv7:149.112.112.112:53,9.9.9.9:53").ok());
}
