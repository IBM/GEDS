/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "GEDSRemoteFileHandle.h"

#include <memory>

#include "FileTransferService.h"
#include "GEDS.h"
#include "GEDSFile.h"
#include "Object.h"

GEDSRemoteFileHandle::GEDSRemoteFileHandle(
    std::shared_ptr<GEDS> gedsService, const geds::Object &object,
    std::shared_ptr<geds::FileTransferService> fileTransferService)
    : GEDSFileHandle(gedsService, object.id.bucket, object.id.key),
      _fileTransferService(fileTransferService), _info(object.info) {
  static auto counter = geds::Statistics::createCounter("GEDSRemoteFileHandle: count");
  *counter += 1;
}

GEDSRemoteFileHandle::~GEDSRemoteFileHandle() {}

bool GEDSRemoteFileHandle::isValid() const { return _isValid; }

absl::StatusOr<size_t> GEDSRemoteFileHandle::size() const {
  // TODO: Update size with remote request.
  return _info.size;
}

absl::Status GEDSRemoteFileHandle::seal() {
  return absl::FailedPreconditionError("Remote files cannot be sealed!");
}

absl::StatusOr<std::shared_ptr<GEDSFileHandle>>
GEDSRemoteFileHandle::factory(std::shared_ptr<GEDS> gedsService, const geds::Object &object) {
  const std::string_view prefix{"geds://"};
  const auto &location = object.info.location;
  if (location.compare(0, prefix.size(), prefix) != 0) {
    return absl::InternalError("Location has invalid prefix for GEDSRemoteFileHandle passed: " +
                               location);
  }
  const auto hostname = location.substr(prefix.size());
  if (hostname.size() == 0) {
    return absl::UnknownError("Invalid hostname");
  }
  auto fileTransferService = gedsService->getFileTransferService(hostname);
  if (!fileTransferService.ok()) {
    return fileTransferService.status();
  }
  return std::shared_ptr<GEDSFileHandle>(
      new GEDSRemoteFileHandle(gedsService, object, fileTransferService.value()));
}
absl::StatusOr<size_t> GEDSRemoteFileHandle::readBytes(uint8_t *bytes, size_t position,
                                                       size_t length) {
  if (length == 0) {
    return 0;
  }
  auto read = _fileTransferService->read(bucket, key, bytes, position, length);
  if (!read.ok()) {
    return read;
  }
  if (*read != length) {
    LOG_DEBUG("Reading ", identifier, " from remote got unexpected length ", *read, " instead of ",
              length);
  }
  *_statistics += *read;
  return *read;
}
