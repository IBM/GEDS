/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/status/status.h>

#include "MetadataService.h"
#include "Object.h"
#include "Platform.h"
#include "Ports.h"

ABSL_FLAG(std::string, serverAddress, "localhost", "Metadata server address.");
ABSL_FLAG(uint16_t, serverPort, defaultMetdataServerPort, "Metadata server port.");
ABSL_FLAG(uint16_t, port, defaultGEDSPort, "Local service port.");

enum class Operation {
  CreateBucket,
  DeleteBucket,
  Create,
  Update,
  Delete,
  List,
};

bool AbslParseFlag(absl::string_view text, Operation *mode, std::string *error) {
  if (text == "cb" || text == "CreateBucket") {
    *mode = Operation::CreateBucket;
    return true;
  }
  if (text == "db" || text == "DeleteBucket") {
    *mode = Operation::DeleteBucket;
    return true;
  }
  if (text == "c" || text == "Create") {
    *mode = Operation::Create;
    return true;
  }
  if (text == "u" || text == "Update") {
    *mode = Operation::Update;
    return true;
  }
  if (text == "d" || text == "Delete") {
    *mode = Operation::Delete;
    return true;
  }
  if (text == "l" || text == "List") {
    *mode = Operation::List;
    return true;
  }
  *error = "Unknown value.";
  return false;
}

std::string AbslUnparseFlag(Operation mode) {
  switch (mode) {
  case Operation::CreateBucket:
    return "CreateBucket";
  case Operation::DeleteBucket:
    return "DeleteBucket";
  case Operation::Create:
    return "Create";
  case Operation::Update:
    return "Update";
  case Operation::Delete:
    return "Delete";
  case Operation::List:
    return "List";
  }
  utility::platform::unreachable();
}

ABSL_FLAG(Operation, operation, Operation::List, "List");
ABSL_FLAG(std::string, bucket, "defaultBucket", "Bucket");
ABSL_FLAG(std::string, key, "defaultKey", "Key");
ABSL_FLAG(std::string, location, "location", "Location");
ABSL_FLAG(uint64_t, size, 0, "Object size.");
ABSL_FLAG(uint64_t, sealedOffset, 0, "Sealed offset.");

int main(int argc, char **argv) {
  absl::ParseCommandLine(argc, argv);
  if (!FLAGS_operation.IsSpecifiedOnCommandLine()) {
    std::cerr << "No operation is specified." << std::endl;
    exit(EXIT_FAILURE);
  }

  Operation operation;
  std::string error;
  if (!AbslParseFlag(FLAGS_operation.CurrentValue(), &operation, &error)) {
    std::cerr << error << std::endl;
    exit(EXIT_FAILURE);
  }

  const auto serverAddress =
      FLAGS_serverAddress.CurrentValue() + ":" + FLAGS_serverPort.CurrentValue();
  std::cout << "Connecting to " << serverAddress << std::endl;
  auto metadataService = geds::MetadataService(serverAddress);

  const auto bucket = FLAGS_bucket.CurrentValue();
  const auto key = FLAGS_key.CurrentValue();
  const auto location = FLAGS_key.CurrentValue();
  const absl::Status result = metadataService.connect();
  if (!result.ok()) {
    std::cerr << "Error connecting to " << serverAddress << ": " << result.message() << std::endl;
    exit(EXIT_FAILURE);
  }

  absl::Status operationResult;
  switch (operation) {
  case Operation::CreateBucket: {
    if (!FLAGS_bucket.IsSpecifiedOnCommandLine()) {
      std::cerr << "No bucket is specified." << std::endl;
      exit(EXIT_FAILURE);
    }
    operationResult = metadataService.createBucket(bucket);
  } break;
  case Operation::DeleteBucket: {
    if (!FLAGS_bucket.IsSpecifiedOnCommandLine()) {
      std::cerr << "No bucket is specified." << std::endl;
      exit(EXIT_FAILURE);
    }
    operationResult = metadataService.deleteBucket(bucket);
  } break;
  case Operation::Create: {
    if (!FLAGS_bucket.IsSpecifiedOnCommandLine()) {
      std::cerr << "No bucket is specified." << std::endl;
      exit(EXIT_FAILURE);
    }
    if (!FLAGS_key.IsSpecifiedOnCommandLine()) {
      std::cerr << "No key is specified." << std::endl;
      exit(EXIT_FAILURE);
    }
    if (!FLAGS_location.IsSpecifiedOnCommandLine()) {
      std::cerr << "No location is specified." << std::endl;
      exit(EXIT_FAILURE);
    }
    uint64_t size = absl::GetFlag(FLAGS_size);
    uint64_t sealedOffset = absl::GetFlag(FLAGS_sealedOffset);
    auto obj =
        geds::Object{geds::ObjectID{bucket, key}, geds::ObjectInfo{location, size, sealedOffset}};
    operationResult = metadataService.createObject(obj);

  } break;
  case Operation::Update: {
    if (!FLAGS_bucket.IsSpecifiedOnCommandLine()) {
      std::cerr << "No bucket is specified." << std::endl;
      exit(EXIT_FAILURE);
    }
    if (!FLAGS_key.IsSpecifiedOnCommandLine()) {
      std::cerr << "No key is specified." << std::endl;
      exit(EXIT_FAILURE);
    }
    if (!FLAGS_location.IsSpecifiedOnCommandLine()) {
      std::cerr << "No location is specified." << std::endl;
      exit(EXIT_FAILURE);
    }
    uint64_t size = absl::GetFlag(FLAGS_size);
    uint64_t sealedOffset = absl::GetFlag(FLAGS_sealedOffset);
    auto obj =
        geds::Object{geds::ObjectID{bucket, key}, geds::ObjectInfo{location, size, sealedOffset}};
    operationResult = metadataService.updateObject(obj);
  } break;
  case Operation::Delete: {
    if (!FLAGS_bucket.IsSpecifiedOnCommandLine()) {
      std::cerr << "No bucket is specified." << std::endl;
      exit(EXIT_FAILURE);
    }
    if (!FLAGS_key.IsSpecifiedOnCommandLine()) {
      std::cerr << "No key is specified." << std::endl;
      exit(EXIT_FAILURE);
    }
    auto obj_id = geds::ObjectID(bucket, key);
    operationResult = metadataService.deleteObject(obj_id);
  } break;
  case Operation::List: {
    if (!FLAGS_bucket.IsSpecifiedOnCommandLine()) {
      std::cerr << "No bucket is specified." << std::endl;
      exit(EXIT_FAILURE);
    }
    if (!FLAGS_key.IsSpecifiedOnCommandLine()) {
      std::cerr << "No key is specified." << std::endl;
      exit(EXIT_FAILURE);
    }
    auto obj_id = geds::ObjectID(bucket, key);
    const auto resp = metadataService.listPrefix(obj_id);
    if (resp.ok()) {
      operationResult = absl::OkStatus();
      for (const auto &obj : *resp) {
        std::cout << obj.id.bucket << "/" << obj.id.key << ": " << obj.info.location
                  << " (size: " << obj.info.size << ", sealed offset: " << obj.info.sealedOffset
                  << ")" << std::endl;
      }
    } else {
      std::cerr << "Unable to query metadata server: " << resp.status().message() << std::endl;
      operationResult = resp.status();
    }
  } break;
  }

  if (!operationResult.ok()) {
    std::cerr << FLAGS_operation.CurrentValue() << ": " << operationResult.message() << std::endl;
    exit(EXIT_FAILURE);
  } else {
    std::cout << FLAGS_operation.CurrentValue() << ": "
              << "Success" << std::endl;
  }

  return EXIT_SUCCESS;
}
