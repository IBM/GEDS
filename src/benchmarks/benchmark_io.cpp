/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/status/status.h>

#include "GEDS.h"
#include "Logging.h"
#include "Platform.h"
#include "Ports.h"

ABSL_FLAG(std::string, address, "localhost:" + std::to_string(defaultMetdataServerPort),
          "Metadata server address.");
ABSL_FLAG(uint16_t, port, defaultGEDSPort, "Local service port.");
ABSL_FLAG(std::string, gedsRoot, "/tmp/GEDS_XXXXXX", "GEDS root folder.");
ABSL_FLAG(std::string, bucket, "benchmark", "Bucket used for benchmarking.");
ABSL_FLAG(size_t, maxThreads, 16, "Maximum number of threads for concurrent downloads.");
ABSL_FLAG(size_t, maxFactor, 18, "Maximum factor.");
ABSL_FLAG(std::string, outputFile, "output.csv", "Filename of the output.");
ABSL_FLAG(bool, sameFile, false, "Access the same file from all threads.");

struct BenchmarkResult {
  size_t payloadSize;
  size_t threads;
  double rate;
  double timeOpenMin;
  double timeOpenMax;
  double timeOpenAvg;
  double timeOpenP25;
  double timeOpenP50;
  double timeOpenP75;

  double timeLastByteMin;
  double timeLastByteMax;
  double timeLastByteAvg;
  double timeLastByteP25;
  double timeLastByteP50;
  double timeLastByteP75;
};

struct Latency {
  double timeOpen;
  double timeLastByte;
};

constexpr size_t KILOBYTE = 1024;
constexpr size_t MEGABYTE = KILOBYTE * KILOBYTE;

size_t getPayloadSize(size_t factor) { return KILOBYTE * (1 << factor); }

void runBenchmarkThread(std::shared_ptr<GEDS> geds, size_t threadId, size_t factor,
                        size_t payloadSize, bool sameFile, std::promise<Latency> &&latency) {
  auto startTime = std::chrono::steady_clock::now();

  std::string key;
  if (sameFile) {
    key = std::to_string(factor) + "-0.data";
  } else {
    key = std::to_string(factor) + "-" + std::to_string(threadId) + ".data";
  }
  std::vector<uint8_t> buffer(payloadSize);
  auto file = geds->open(FLAGS_bucket.CurrentValue(), key);
  if (file.ok()) {
    auto openTime = std::chrono::steady_clock::now();
    auto status = file->read(buffer, 0, payloadSize);
    auto lastByteTime = std::chrono::steady_clock::now();
    constexpr auto milliseconds = 1e6;
    latency.set_value(
        {/* timeOpen */ (double)std::chrono::duration_cast<std::chrono::microseconds>(openTime -
                                                                                      startTime)
                 .count() /
             milliseconds,
         /* timeLastByte */ (double)std::chrono::duration_cast<std::chrono::microseconds>(
             lastByteTime - startTime)
                 .count() /
             milliseconds});

    if (!status.ok()) {
      auto message = "Error: " + std::string{status.status().message()};
      LOG_ERROR(message);
      throw new std::runtime_error(message);
    }
  }
}

BenchmarkResult benchmark(std::shared_ptr<GEDS> geds, size_t factor, size_t numThreads) {
  bool sameFile = absl::GetFlag(FLAGS_sameFile);
  auto threads = std::vector<std::thread>(numThreads);
  auto payloadSize = getPayloadSize(factor);
  auto futures = std::vector<std::future<Latency>>(numThreads);
  auto startTime = std::chrono::steady_clock::now();
  for (size_t i = 0; i < numThreads; i++) {
    std::promise<Latency> p;
    futures[i] = p.get_future();
    threads[i] =
        std::thread(runBenchmarkThread, geds, i, factor, payloadSize, sameFile, std::move(p));
  }
  for (auto &t : threads) {
    t.join();
  }
  auto endTime = std::chrono::steady_clock::now();
  auto seconds =
      (double)std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count() /
      MEGABYTE;
  auto rate = double(numThreads) * payloadSize / (seconds) / MEGABYTE;

  auto timeOpen = std::vector<double>();
  timeOpen.reserve(numThreads);
  auto timeLastByte = std::vector<double>();
  timeLastByte.reserve(numThreads);

  double timeOpenAvg = 0;
  double timeLastByteAvg = 0;
  for (auto &f : futures) {
    auto l = f.get();
    timeOpen.push_back(l.timeOpen);
    timeOpenAvg += l.timeOpen;
    timeLastByte.push_back(l.timeLastByte);
    timeLastByteAvg += l.timeLastByte;
  }
  std::sort(timeOpen.begin(), timeOpen.end());
  std::sort(timeLastByte.begin(), timeLastByte.end());

  return BenchmarkResult{
      payloadSize,
      numThreads,
      rate,
      /* timeOpenMin */ timeOpen[0],
      /* timeOpenMax */ timeOpen[numThreads - 1],
      /* timeOpenAvg */ timeOpenAvg / numThreads,
      /* timeOpenP25 */ timeOpen[std::max(0.25 * numThreads - 1.0, 0.0)], // NOLINT
      /* timeOpenP50 */ timeOpen[std::max(0.50 * numThreads - 1.0, 0.0)], // NOLINT
      /* timeOpenP75 */ timeOpen[std::max(0.75 * numThreads - 1.0, 0.0)], // NOLINT
      /* timeLastByteMin */ timeLastByte[0],
      /* timeLastByteMax */ timeLastByte[numThreads - 1],
      /* timeLastByteAvg */ timeLastByteAvg / numThreads,
      /* timeLastByteP25 */ timeLastByte[std::max(0.25 * numThreads - 1.0, 0.0)],  // NOLINT
      /* timeLastByteP50 */ timeLastByte[std::max(0.50 * numThreads - 1.0, 0.0)],  // NOLINT
      /* timeLastByteP75 */ timeLastByte[std::max(0.75 * numThreads - 1.0, 0.0)]}; // NOLINT
}

int main(int argc, char **argv) {
  absl::ParseCommandLine(argc, argv);

  std::ofstream f(FLAGS_outputFile.CurrentValue());
  if (!f.is_open()) {
    std::cerr << "Unable to open " << FLAGS_outputFile.CurrentValue() << " for writing."
              << std::endl;
    exit(EXIT_FAILURE);
  }
  f << "Payload Size,Thread Count,Throughput,"
    << "Time Open [avg],Time Open [min],Time Open [p25],Time Open [p50], Time Open [p75],"
    << "Time Open [max],"
    << "Time Last Byte [avg],Time Last Byte [min],Time Last Byte [p25],Time Last Byte [p50],"
    << "Time Last Byte [p75],Time Last Byte [max]" << std::endl;
  for (size_t i = 0; i <= absl::GetFlag(FLAGS_maxFactor); i++) {
    for (size_t j = 1; j < absl::GetFlag(FLAGS_maxThreads); j++) {
      // Create a new GEDS instance for each iteration to measure performance.
      auto geds = GEDS::factory(FLAGS_address.CurrentValue(), FLAGS_gedsRoot.CurrentValue(),
                                std::nullopt, absl::GetFlag(FLAGS_port));
      auto status = geds->start();
      if (!status.ok()) {
        std::cout << "Unable to start GEDS:" << status.message() << std::endl;
        exit(EXIT_FAILURE);
      }

      auto result = benchmark(geds, i, j);
      std::cout << result.payloadSize << ": " << result.threads << " " << result.rate << " MB/s"
                << std::endl;
      f << result.payloadSize << "," << result.threads << "," << result.rate << ","
        << result.timeOpenAvg << "," << result.timeOpenMin << "," << result.timeOpenP25 << ","
        << result.timeOpenP50 << "," << result.timeOpenP75 << "," << result.timeOpenMax << ","
        << result.timeLastByteAvg << "," << result.timeLastByteMin << "," << result.timeLastByteP25
        << "," << result.timeLastByteP50 << "," << result.timeLastByteP75 << ","
        << result.timeLastByteMax << std::endl;

      (void)geds->stop();
    }
  }
  f.close();

  return EXIT_SUCCESS;
}
