/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "GEDSFile.h"

#include "GEDS.h"
#include "GEDSFileHandle.h"
#include "Logging.h"

GEDSFile::GEDSFile(std::shared_ptr<GEDS> geds, std::shared_ptr<GEDSFileHandle> fileHandle)
    : _geds(geds), _fileHandle(fileHandle) {
  _fileHandle->increaseOpenCount();
}

GEDSFile::GEDSFile(const GEDSFile &other) : GEDSFile(other._geds, other._fileHandle) {}

GEDSFile::GEDSFile(GEDSFile &&other) {
  _geds = std::move(other._geds);
  _fileHandle = std::move(other._fileHandle);
  other._fileHandle = nullptr;
}

std::optional<std::string> GEDSFile::metadata() const { return _fileHandle->metadata(); }
absl::Status GEDSFile::setMetadata(std::optional<std::string> metadata, bool seal) {
  return _fileHandle->setMetadata(metadata, seal);
}

GEDSFile &GEDSFile::operator=(const GEDSFile &other) {
  if (this != &other) {
    _fileHandle = other._fileHandle;
    _fileHandle->increaseOpenCount();
  }
  return *this;
}

GEDSFile &GEDSFile::operator=(GEDSFile &&other) {
  if (this != &other) {
    _geds = std::move(other._geds);
    _fileHandle = std::move(other._fileHandle);
    other._fileHandle = nullptr;
  }
  return *this;
}

bool GEDSFile::operator==(const GEDSFile &other) const {
  return _fileHandle->bucket == other._fileHandle->bucket &&
         _fileHandle->key == other._fileHandle->key;
}

GEDSFile::~GEDSFile() {
  if (_fileHandle != nullptr) {
    _fileHandle->decreaseOpenCount();
  }
}

size_t GEDSFile::size() const { return _fileHandle->size().value_or(0); }

const std::string &GEDSFile::bucket() const { return _fileHandle->bucket; }
const std::string &GEDSFile::key() const { return _fileHandle->key; }
const std::string &GEDSFile::identifier() const { return _fileHandle->identifier; };
const std::shared_ptr<GEDSFileHandle> GEDSFile::fileHandle() const { return _fileHandle; }

bool GEDSFile::isWriteable() const { return _fileHandle->isWriteable(); }

absl::StatusOr<size_t> GEDSFile::readBytes(uint8_t *bytes, size_t position, size_t length,
                                           bool retry) {
  auto status = _fileHandle->readBytes(bytes, position, length);
  if (status.ok() || !retry) {
    return status;
  }
  auto fh = _geds->reopen(_fileHandle);
  // Unable to reopen: Return error.
  if (!fh.ok()) {
    return fh.status();
  }
  _fileHandle = *fh;
  return readBytes(bytes, position, length, false);
}

absl::Status GEDSFile::writeBytes(const uint8_t *bytes, size_t position, size_t length) {
  return _fileHandle->writeBytes(bytes, position, length);
}

absl::StatusOr<int> GEDSFile::rawFd() const { return _fileHandle->rawFd(); }

absl::Status GEDSFile::copyTo(GEDSFile &destination) const {
  auto status = _fileHandle->download(destination._fileHandle);
  if (!status.ok()) {
    return status;
  }
  return absl::OkStatus();
}

absl::Status GEDSFile::seal() { return _fileHandle->seal(); }

absl::Status GEDSFile::truncate(size_t size) { return _fileHandle->truncate(size); }
