/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <unistd.h>
#include <vector>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/status/status.h>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "GEDS.h"
#include "GEDSFile.h"
#include "GEDSFileStatus.h"
#include "Logging.h"
#include "Ports.h"
#include "Statistics.h"

ABSL_FLAG(std::string, address, "10.40.1.4:50003", "Metadata server address.");
ABSL_FLAG(uint16_t, localPort, defaultGEDSPort, "Local service port.");
ABSL_FLAG(std::string, gedsRoot, "/tmp/GEDS_XXXXXX", "GEDS root folder.");
ABSL_FLAG(std::string, bucket, "benchmark", "Bucket used for benchmarking.");
ABSL_FLAG(size_t, numExecutors, 4, "The number of executors.");
ABSL_FLAG(size_t, numTasksExecutor, 100, "The of tasks per executor.");
ABSL_FLAG(size_t, numFiles, 1000, "The number of shuffle files to be created.");

void makeSubscriberStream(std::shared_ptr<GEDS> geds, const std::string subscriber_id) {
  auto result = geds->subscribeStream(subscriber_id);
}

void runSubscriberThread(std::shared_ptr<GEDS> geds, const std::string &bucket) {
  boost::uuids::uuid uuid_generated = boost::uuids::random_generator()();
  const auto uuid = boost::lexical_cast<std::string>(uuid_generated);

  auto subscriberTread = std::thread(makeSubscriberStream, geds, uuid);
  subscriberTread.detach();

  auto subscriptionObject = geds::SubscriptionEvent{bucket, "", geds::rpc::BUCKET};
  auto testResult = geds->subscribe(subscriptionObject, uuid);
}

absl::StatusOr<size_t> runTask(std::shared_ptr<GEDS> geds, const std::string &bucket, size_t nFiles) {
  static auto openStatistics =
      geds::Statistics::createNanoSecondHistogram("Without PubSub Read: Open");
  static auto readStatistics =
      geds::Statistics::createNanoSecondHistogram("Without PubSub Read: Read");

  size_t totalBytesRead = 0;
  for (size_t i = 0; i < nFiles; i++) {
    auto timerBegin = std::chrono::high_resolution_clock::now();
    auto file = geds->open(bucket, std::to_string(i));
    const auto readSize = file->size();
    *openStatistics += std::chrono::duration_cast<std::chrono::nanoseconds>(
                           std::chrono::high_resolution_clock::now() - timerBegin)
                           .count();
    if (!file.ok()) {
      LOG_ERROR("Unable to open file: ", file.status().message());
      return file.status();
    }

    timerBegin = std::chrono::high_resolution_clock::now();
    std::vector<uint8_t> buffer(readSize);
    auto st = file->read(buffer, 0, readSize);
    *readStatistics += std::chrono::duration_cast<std::chrono::nanoseconds>(
                           std::chrono::high_resolution_clock::now() - timerBegin)
                           .count();
    if (!st.ok()) {
      LOG_ERROR("Unable to read file: ", file.status().message());
      return st.status();
    }
    totalBytesRead += *st;
  }
  return totalBytesRead;
}

void runExecutorThread(std::shared_ptr<GEDS> geds, const std::string &bucket, size_t executorId,
                       size_t nTasks, size_t nFiles) {
  size_t totalBytesRead = 0;
  for (size_t i = 0; i < nTasks; i++) {
    auto success = runTask(geds, bucket, nFiles);
    if (!success.ok()) {
      LOG_ERROR("Unable to execute task: ", success.status().message());
      return;
    }
    totalBytesRead += *success;
  }
  LOG_DEBUG("Executor ", executorId, " finished all ", nTasks,
            " tasks. Total bytes read: ", totalBytesRead);
}

int main(int argc, char **argv) {
  absl::ParseCommandLine(argc, argv);
  auto config = GEDSConfig(FLAGS_address.CurrentValue());
  config.port = absl::GetFlag(FLAGS_localPort);
  config.localStoragePath = FLAGS_gedsRoot.CurrentValue();
  auto geds = GEDS::factory(config);
  absl::Status status;
  status = geds->start();
  if (!status.ok()) {
    LOG_ERROR("Unable to start GEDS: ", status.message());
    exit(EXIT_FAILURE);
  }

  const auto bucketName = FLAGS_bucket.CurrentValue();
  const auto nExecutors = absl::GetFlag(FLAGS_numExecutors);
  const auto nTasksExecutor = absl::GetFlag(FLAGS_numTasksExecutor);
  const auto nFiles = absl::GetFlag(FLAGS_numFiles);

  std::cout << "Starting With PubSub Read benchmark."
            << "\n"
            << "Bucket:             " << bucketName << "\n"
            << "# executors:        " << nExecutors << "\n"
            << "# tasks/executor:   " << nTasksExecutor << std::endl;

  auto subscriberThreads = std::vector<std::thread>();
  subscriberThreads.reserve(nExecutors);
  for (size_t i = 0; i < nExecutors; i++) {
    subscriberThreads.emplace_back(
        std::thread(runSubscriberThread, geds, bucketName));
  }
  for (auto &t : subscriberThreads) {
    t.join();
  }

  std::string input;
  std::getline(std::cin, input);

  auto startTime = std::chrono::steady_clock::now();

  auto executorThreads = std::vector<std::thread>();
  executorThreads.reserve(nExecutors);
  for (size_t i = 0; i < nExecutors; i++) {
    executorThreads.emplace_back(
        std::thread(runExecutorThread, geds, bucketName, i, nTasksExecutor, nFiles));
  }
  for (auto &t : executorThreads) {
    t.join();
  }

  auto endTime = std::chrono::steady_clock::now();
  std::cout
      << "With PubSub read took: "
      << (double)std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count()
      << " ms" << std::endl;

  (void)geds->stop();
  return EXIT_SUCCESS;
}
