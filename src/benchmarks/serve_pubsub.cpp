/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <unistd.h>
#include <vector>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/status/status.h>

#include "GEDS.h"
#include "Logging.h"
#include "Ports.h"

constexpr size_t KILOBYTE = 1024;
constexpr size_t MEGABYTE = KILOBYTE * KILOBYTE;

ABSL_FLAG(std::string, address, "10.40.1.4:50003", "Metadata server address.");
ABSL_FLAG(uint16_t, localPort, defaultGEDSPort, "Local service port.");
ABSL_FLAG(std::string, gedsRoot, "/tmp/GEDS_XXXXXX", "GEDS root folder.");
ABSL_FLAG(std::string, bucket, "benchmark", "Bucket used for benchmarking.");
ABSL_FLAG(size_t, numFiles, 1000, "The number of shuffle files to be created.");
ABSL_FLAG(size_t, fileSizeMB, 1, "The file size for each shuffle file (MegaBytes).");
ABSL_FLAG(size_t, numExecutors, 10, "The number of executors.");

void createFile(std::shared_ptr<GEDS> &geds, const std::string &bucket, size_t fileNum,
                size_t sizeMB) {
  auto fname = std::to_string(fileNum);
  auto file = geds->create(bucket, fname);
  absl::Status status;
  status = file->truncate(sizeMB * MEGABYTE);
  if (!status.ok()) {
    LOG_ERROR("Unable to create file ", fname, ". Reason: ", status.message());
    exit(EXIT_FAILURE);
  }

  std::mt19937 gen;
  gen.seed(fileNum);
  std::uniform_int_distribution<uint8_t> dist;
  auto buffer = std::vector<uint8_t>(MEGABYTE);
  for (size_t i = 0; i < sizeMB; i++) {
    std::generate(begin(buffer), end(buffer), [&dist, &gen] { return dist(gen); });
  }
  status = file->seal();
  if (!status.ok()) {
    LOG_ERROR("Unable to create file ", fname, ". Reason: ", status.message());
    exit(EXIT_FAILURE);
  }
}

void runExecutorThread(std::shared_ptr<GEDS> geds, const std::string &bucket, size_t executorId,
                       size_t filesPerExecutor, size_t sizeMB) {
  for (size_t i = 0; i < filesPerExecutor; i++) {
    createFile(geds, bucket, i + (executorId * filesPerExecutor), sizeMB);
  }

  LOG_DEBUG("Executor ", executorId, " finished all ", filesPerExecutor, " files created.");
}

int main(int argc, char **argv) {
  absl::ParseCommandLine(argc, argv);
  auto config = GEDSConfig(FLAGS_address.CurrentValue());
  config.port = absl::GetFlag(FLAGS_localPort);
  config.localStoragePath = FLAGS_gedsRoot.CurrentValue();
  auto geds = GEDS::factory(config);
  auto status = geds->start();
  if (!status.ok()) {
    std::cout << "Unable to start GEDS:" << status.message() << std::endl;
    exit(EXIT_FAILURE);
  }

  const auto prefix = FLAGS_bucket.CurrentValue();
  const auto nFiles = absl::GetFlag(FLAGS_numFiles);
  const auto sizeMB = absl::GetFlag(FLAGS_fileSizeMB);
  const auto nExecutors = absl::GetFlag(FLAGS_numExecutors);
  const auto filesPerExecutor = nFiles / nExecutors;

  auto executorThreads = std::vector<std::thread>();
  executorThreads.reserve(nExecutors);
  for (size_t i = 0; i < nExecutors; i++) {
    executorThreads.emplace_back(
        std::thread(runExecutorThread, geds, prefix, i, filesPerExecutor, sizeMB));
  }
  for (auto &t : executorThreads) {
    t.join();
  }

  sleep(10000000);
  return EXIT_SUCCESS;
}
