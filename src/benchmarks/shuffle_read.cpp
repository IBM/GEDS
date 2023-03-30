/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
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

#include "GEDS.h"
#include "GEDSFile.h"
#include "GEDSFileStatus.h"
#include "Logging.h"
#include "Ports.h"
#include "Statistics.h"

ABSL_FLAG(std::string, address, "localhost:" + std::to_string(defaultMetdataServerPort),
          "Metadata server address.");
ABSL_FLAG(uint16_t, localPort, defaultGEDSPort, "Local service port.");
ABSL_FLAG(std::string, gedsRoot, "/tmp/GEDS_XXXXXX", "GEDS root folder.");
ABSL_FLAG(std::string, bucket, "benchmark", "Bucket used for benchmarking.");
ABSL_FLAG(size_t, numFiles, 75, "The number of shuffle files per executor.");
ABSL_FLAG(size_t, numExecutors, 4, "The number of executors.");
ABSL_FLAG(size_t, numTasksExecutor, 100, "The of tasks per executor.");
ABSL_FLAG(size_t, numThreadsExecutor, 1, "The number of threads accessing each file.");

absl::StatusOr<size_t> runTask(std::shared_ptr<GEDS> geds, const std::string &bucket, size_t taskId,
                               size_t readSize, size_t nFiles) {
  static auto openStatistics = geds::Statistics::createNanoSecondHistogram("Shuffle Read: Open");
  static auto readStatistics = geds::Statistics::createNanoSecondHistogram("Shuffle Read: Read");

  size_t totalBytesRead = 0;
  std::vector<uint8_t> buffer(readSize);
  for (size_t i = 0; i < nFiles; i++) {
    auto timerBegin = std::chrono::high_resolution_clock::now();
    auto file = geds->open(bucket, std::to_string(i));
    *openStatistics += std::chrono::duration_cast<std::chrono::nanoseconds>(
                           std::chrono::high_resolution_clock::now() - timerBegin)
                           .count();
    if (!file.ok()) {
      LOG_ERROR("Unable to open file: ", file.status().message());
      return file.status();
    }
    timerBegin = std::chrono::high_resolution_clock::now();
    auto st = file->read(buffer, taskId * readSize, readSize);
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
                       size_t nTasks, size_t /* nThreads */, size_t nFiles, size_t sizePerRead) {
  std::vector<size_t> tasks(nTasks);
  for (size_t i = 0; i < nTasks; i++) {
    tasks[i] = i + (executorId * nTasks);
  }

  // Use a deterministic seed.
  std::mt19937 gen;
  gen.seed(executorId);
  std::shuffle(std::begin(tasks), std::end(tasks), gen);

  size_t totalBytesRead = 0;
  for (auto taskId : tasks) {
    auto success = runTask(geds, bucket, taskId, sizePerRead, nFiles);
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
  const auto nFiles = absl::GetFlag(FLAGS_numFiles);
  const auto nExecutors = absl::GetFlag(FLAGS_numExecutors);
  const auto nTasksExecutor = absl::GetFlag(FLAGS_numTasksExecutor);
  const auto nThreadsExecutor = absl::GetFlag(FLAGS_numThreadsExecutor);

  auto folderStatus = geds->listAsFolder(bucketName, "");
  if (!folderStatus.ok()) {
    LOG_ERROR("Unable to list bucket ", bucketName, " reason: ", folderStatus.status().message());
    exit(EXIT_FAILURE);
  }
  size_t fileCount = std::count_if(folderStatus->begin(), folderStatus->end(),
                                   [](const GEDSFileStatus &f) { return !f.isDirectory; });
  if (fileCount < nFiles) {
    LOG_ERROR("Invalid number of files: Expected ", nFiles, " got ", fileCount);
    exit(EXIT_FAILURE);
  }

  auto startTime = std::chrono::steady_clock::now();

  auto maxSize = std::max_element(
      folderStatus->begin(), folderStatus->end(),
      [](const GEDSFileStatus &a, const GEDSFileStatus &b) { return a.size < b.size; });
  auto sizePerRead = maxSize->size / (nExecutors * nTasksExecutor);

  std::cout << "Starting shuffle Read benchmark."
            << "\n"
            << "Bucket:             " << bucketName << "\n"
            << "# files:            " << nFiles << "\n"
            << "# executors:        " << nExecutors << "\n"
            << "# tasks/executor:   " << nTasksExecutor << "\n"
            << "# threads/executor: " << nThreadsExecutor << "\n"
            << "size per read:      " << sizePerRead << std::endl;

  auto executorThreads = std::vector<std::thread>();
  executorThreads.reserve(nExecutors);
  for (size_t i = 0; i < nExecutors; i++) {
    executorThreads.emplace_back(std::thread(runExecutorThread, geds, bucketName, i, nTasksExecutor,
                                             nThreadsExecutor, nFiles, sizePerRead));
  }
  for (auto &t : executorThreads) {
    t.join();
  }

  auto endTime = std::chrono::steady_clock::now();
  std::cout
      << "Shuffle read took: "
      << (double)std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count()
      << " ms" << std::endl;

  (void)geds->stop();
  return EXIT_SUCCESS;
}
