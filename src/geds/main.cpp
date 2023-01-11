/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#include <csignal>
#include <cstdlib>
#include <iostream>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/status/status.h>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "GEDS.h"
#include "Platform.h"
#include "Ports.h"

ABSL_FLAG(std::string, serverAddress, "localhost", "Metadata server address.");
;
ABSL_FLAG(uint16_t, serverPort, defaultMetdataServerPort, "Metadata server port.");
ABSL_FLAG(uint16_t, port, defaultGEDSPort, "Local service port.");
ABSL_FLAG(std::string, gedsRoot, "/tmp/GEDS_XXXXXX", "GEDS root folder.");
ABSL_FLAG(std::string, downloadFile, "", "File to download.");

static bool running = false;
static std::shared_ptr<GEDS> gedsInstance = NULL;
void stopHandler(int /* unused signum */) {
  if (gedsInstance != NULL) {
    (void)gedsInstance->stop();
    running = false;
  }
}

void serveFile(std::shared_ptr<GEDS> gedsInstance) {
  std::string bucket = utility::platform::getHostName();
  auto bucketStatus = gedsInstance->createBucket(bucket);
  if (!bucketStatus.ok()) {
    std::cerr << "Unable to create bucket " << bucket << std::endl;
  }
  std::string key = "testfile";
  auto fileStatus = gedsInstance->create(bucket, key);
  if (!fileStatus.ok()) {
    std::cerr << "Unable to create file " << bucket << "/" << key << ": "
              << fileStatus.status().message() << std::endl;
    exit(EXIT_FAILURE);
  }
  auto file = fileStatus.value();
  std::string text = "Hello from " + bucket + "\n";
  (void)file.write(text.c_str(), 0, text.size());
  std::cout << "Created file " << bucket << "/" << key << " at path '"
            << gedsInstance->getLocalPath(bucket, key) << "'." << std::endl;
  // ToDo: FIXME.
  auto sealStatus = file.seal();
  if (!sealStatus.ok()) {
    std::cerr << "Could not seal file: " << sealStatus.message() << std::endl;
    exit(EXIT_FAILURE);
  }

  while (running) {
    sleep(1);
  }
}

void downloadFile(std::shared_ptr<GEDS> gedsInstance, const std::string &bucket,
                  const std::string &key) {
  auto fileStatus = gedsInstance->open(bucket, key);
  if (!fileStatus.ok()) {
    std::cerr << "Unable to open file '" << bucket << "/" << key
              << "': " << fileStatus.status().message() << std::endl;
    exit(EXIT_FAILURE);
  }
  auto file = fileStatus.value();
  std::vector<uint8_t> buffer;
  auto readStatus = file.read(buffer, 0, 0, file.size());
  if (!readStatus.ok()) {
    std::cerr << "Unable to read from file: " << readStatus.status().message() << std::endl;
    return;
  }
  if (readStatus.value() != file.size()) {
    std::cerr << "File has unexpected length!" << std::endl;
  }
  std::cout << "Read file with length " << std::to_string(readStatus.value()) << std::endl;
  if (key.ends_with(".txt")) {
    std::string message{reinterpret_cast<const char *>(&buffer[0])}; // NOLINT
    std::cout << "The file " << file.bucket() << "/" << file.key() << " has the following content: "
              << "\n"
              << message << std::endl;
  }
}

#define BIG_FILE_SIZE 1024 * 1024 * 1024

void createBigFile(std::shared_ptr<GEDS> gedsInstance, std::string bucket, std::string key) {
  auto bucketStatus = gedsInstance->createBucket(bucket);
  if (!bucketStatus.ok()) {
    std::cerr << "Unable to create bucket " << bucket << std::endl;
    exit(EXIT_FAILURE);
  }

  auto fileStatus = gedsInstance->create(bucket, key);
  if (!fileStatus.ok()) {
    std::cerr << "Unable to create file " << bucket << "/" << key << ": "
              << fileStatus.status().message() << std::endl;
    exit(EXIT_FAILURE);
  }
  auto file = fileStatus.value();

  int size = BIG_FILE_SIZE;
  int off = 0;
  while (size > 0) {
    char buffer[40960];

    (void)file.write(buffer, off, 40960);
    off += 40960;
    size -= 40960;
  }
  std::cout << "Created file " << bucket << "/" << key << " at path '"
            << gedsInstance->getLocalPath(bucket, key) << "'." << std::endl;

  auto sealStatus = file.seal();
  if (!sealStatus.ok()) {
    std::cerr << "Could not seal big file: " << sealStatus.message() << std::endl;
    exit(EXIT_FAILURE);
  }
  std::cout << "Created Big File, size: " << file.size() << std::endl;
}

int main(int argc, char **argv) {
  absl::ParseCommandLine(argc, argv);
  auto serverAddress = FLAGS_serverAddress.CurrentValue() + ":" + FLAGS_serverPort.CurrentValue();
  gedsInstance = GEDS::factory(serverAddress, FLAGS_gedsRoot.CurrentValue(), std::nullopt,
                               absl::GetFlag(FLAGS_port));

  signal(SIGINT, stopHandler);
  auto status = gedsInstance->start();
  if (!status.ok()) {
    std::cout << "Unable to start GEDS: " << status.message() << std::endl;
    exit(EXIT_FAILURE);
  }

  running = true;
  std::cout << "GEDS is running." << std::endl;
  if (FLAGS_downloadFile.CurrentValue().empty()) {
    const std::string bigfile_bucket = "nase";
    const std::string bigfile_key = "baer";
    std::cout << "Creating big file at " << bigfile_bucket << "/" << bigfile_key << std::endl;
    createBigFile(gedsInstance, bigfile_bucket, bigfile_key);
    serveFile(gedsInstance);
  } else {
    const auto &path = FLAGS_downloadFile.CurrentValue();
    auto loc = path.find("/");
    std::string bucket = path.substr(0, loc);
    std::string key = path.substr(loc + 1);

    downloadFile(gedsInstance, bucket, key);
  }

  return EXIT_SUCCESS;
}
