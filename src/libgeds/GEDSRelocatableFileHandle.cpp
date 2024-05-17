/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "GEDSRelocatableFileHandle.h"

#include "GEDS.h"
#include "GEDSFile.h"
#include "Logging.h"

std::shared_ptr<GEDSFileHandle>
GEDSRelocatableFileHandle::factory(std::shared_ptr<GEDS> gedsService,
                                   std::shared_ptr<GEDSFileHandle> wrapped) {
  return std::shared_ptr<GEDSFileHandle>(new GEDSRelocatableFileHandle(gedsService, wrapped));
}

bool GEDSRelocatableFileHandle::isRelocatable() const {
  auto lock = lockShared();
  return _fileHandle->isRelocatable();
}

absl::StatusOr<size_t> GEDSRelocatableFileHandle::size() const {
  auto lock = lockShared();
  return _fileHandle->size();
}

size_t GEDSRelocatableFileHandle::localStorageSize() const {
  auto lock = lockShared();
  return _fileHandle->localStorageSize();
}

size_t GEDSRelocatableFileHandle::localMemorySize() const {
  auto lock = lockShared();
  return _fileHandle->localMemorySize();
}

bool GEDSRelocatableFileHandle::isWriteable() const {
  auto lock = lockShared();
  // ToDo: Download relocated file again to make it writeable.
  return _fileHandle->isWriteable();
}

std::optional<std::string> GEDSRelocatableFileHandle::metadata() const {
  auto lock = lockFile();
  return _fileHandle->metadata();
};

absl::Status GEDSRelocatableFileHandle::setMetadata(std::optional<std::string> metadata,
                                                    bool seal) {
  auto lock = lockFile();
  return _fileHandle->setMetadata(metadata, seal);
}

absl::StatusOr<size_t> GEDSRelocatableFileHandle::readBytes(uint8_t *bytes, size_t position,
                                                            size_t length) {
  GEDSFileHandle *oldFh;
  {
    auto lock = lockShared();
    oldFh = _fileHandle.get();
    auto success = _fileHandle->readBytes(bytes, position, length);
    if (success.ok()) {
      return *success;
    }
  }
  // Reopen in case of read failures.
  {
    auto lock = lockFile();
    auto ioLock = lockExclusive();
    if (_fileHandle.get() != oldFh) {
      // The file has already been reopened.
      return _fileHandle->readBytes(bytes, position, length);
    }
    LOG_INFO("Reopening file ", identifier);
    // Force lookup in MDS.
    auto newFh = _gedsService->reopenFileHandle(bucket, key, true);
    if (!newFh.ok()) {
      LOG_INFO("Unable to reopen file: ", identifier, " reason: ", newFh.status().message());
      return newFh.status();
    }
    // LOG_INFO("Reopened file", identifier);
    _fileHandle = *newFh;
    return _fileHandle->readBytes(bytes, position, length);
  }
}

absl::Status GEDSRelocatableFileHandle::writeBytes(const uint8_t *bytes, size_t position,
                                                   size_t length) {
  auto lock = lockShared();
  return _fileHandle->writeBytes(bytes, position, length);
}

absl::Status GEDSRelocatableFileHandle::write(std::istream &stream, size_t position,
                                              std::optional<size_t> lengthOptional) {
  auto lock = lockShared();
  return _fileHandle->write(stream, position, lengthOptional);
}

absl::Status GEDSRelocatableFileHandle::truncate(size_t targetSize) {
  auto lock = lockExclusive();
  return _fileHandle->truncate(targetSize);
}

absl::Status GEDSRelocatableFileHandle::seal() {
  auto lock = lockExclusive();
  return _fileHandle->seal();
}

void GEDSRelocatableFileHandle::notifyUnused() {
  auto lock = lockFile();
  auto lockIo = lockExclusive();
  return _fileHandle->notifyUnused();
};

absl::StatusOr<int> GEDSRelocatableFileHandle::rawFd() const {
  auto lock = lockShared();
  return _fileHandle->rawFd();
}

absl::StatusOr<std::shared_ptr<GEDSFileHandle>> GEDSRelocatableFileHandle::relocate() {
  auto lock = lockFile();
  auto lockIo = lockExclusive();

  auto newFh = _fileHandle->relocate();
  if (newFh.ok()) {
    _fileHandle = *newFh;
  }
  return shared_from_this();
}
