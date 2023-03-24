/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
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

ABSL_FLAG(std::string, address, "localhost:" + std::to_string(defaultMetdataServerPort),
          "Metadata server address.");
ABSL_FLAG(uint16_t, localPort, defaultGEDSPort, "Local service port.");
ABSL_FLAG(std::string, gedsRoot, "/tmp/GEDS_XXXXXX", "GEDS root folder.");
ABSL_FLAG(std::string, bucket, "benchmark", "Bucket used for benchmarking.");
ABSL_FLAG(size_t, numFiles, 75, "The number of shuffle files to be created.");
ABSL_FLAG(size_t, fileSizeMB, 18, "The file size for each shuffle file (MegaBytes).");

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

int main(int argc, char **argv) {
  absl::ParseCommandLine(argc, argv);
  auto geds = GEDS::factory(FLAGS_address.CurrentValue(), FLAGS_gedsRoot.CurrentValue(),
                            std::nullopt, absl::GetFlag(FLAGS_localPort));
  auto status = geds->start();
  if (!status.ok()) {
    std::cout << "Unable to start GEDS:" << status.message() << std::endl;
    exit(EXIT_FAILURE);
  }

  const auto prefix = FLAGS_bucket.CurrentValue();
  const auto nFiles = absl::GetFlag(FLAGS_numFiles);
  const auto sizeMB = absl::GetFlag(FLAGS_fileSizeMB);
  for (size_t i = 0; i < nFiles; i++) {
    createFile(geds, prefix, i, sizeMB);
  }
  sleep(10000000);
  return EXIT_SUCCESS;
}
