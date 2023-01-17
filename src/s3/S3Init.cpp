/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "S3Init.h"

#include <atomic>
#include <chrono>
#include <memory>

#include <aws/core/Aws.h>
#include <thread>

static std::shared_ptr<Aws::SDKOptions> awsSDKoptions;
static std::atomic<bool> awsSDKinitialized{false};
static std::atomic<bool> awaitSDKInitialization{true};

namespace geds::s3 {
void Init() {
  bool initExpected = false;
  bool notYetInitialized = awsSDKinitialized.compare_exchange_strong(initExpected, true);
  if (notYetInitialized) {
    awsSDKoptions = std::make_shared<Aws::SDKOptions>();
    Aws::InitAPI(*awsSDKoptions);
    awaitSDKInitialization = false;
  } else {
    while (awaitSDKInitialization) {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(100ms);
    }
  }
}

void Shutdown() {
  bool initExpected = true;
  bool notYetInitialized = awsSDKinitialized.compare_exchange_strong(initExpected, false);
  if (notYetInitialized) {
    awsSDKoptions = nullptr;
  }
}

} // namespace geds::s3
