/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "GEDSFileHandle.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "GEDS.h"
#include "GEDSFile.h"
#include "GEDSInternal.h"
#include "Logging.h"
#include "Statistics.h"

GEDSFileHandle::GEDSFileHandle(std::shared_ptr<GEDS> gedsService, std::string bucketArg,
                               std::string keyArg, std::optional<std::string> metadataArg)
    : enable_shared_from_this(), bucket(std::move(bucketArg)), key(std::move(keyArg)),
      identifier(bucket + "/" + key), _metadata(std::move(metadataArg)), _gedsService(gedsService) {
  LOG_DEBUG("Created filehandle ", identifier, " with metadata ",
            _metadata.has_value() ? std::to_string(_metadata.value().size()) + "bytes" : "<none>");
}

GEDSFileHandle::~GEDSFileHandle() {
  if (_openCount.load() != 0) {
    static auto danglingRefsCounter =
        geds::Statistics::createCounter("GEDS: closed filehandles with dangling references");
    *danglingRefsCounter += 1;
    LOG_ERROR("The file handle " + identifier + " has still dangling references.");
  }
}

int64_t GEDSFileHandle::openCount() const { return _openCount; }
void GEDSFileHandle::increaseOpenCount() {
  auto lock = lockFile();
  _openCount++;
  _lastOpened = std::chrono::system_clock::now();
}
void GEDSFileHandle::decreaseOpenCount() {
  auto lock = lockFile();

  _openCount--;
  _lastReleased = std::chrono::system_clock::now();
  if (_openCount == 0) {
    notifyUnused();
  }
}

bool GEDSFileHandle::isValid() const {
  auto lock = lockFile();
  return _isValid;
}

absl::StatusOr<std::shared_ptr<GEDSFileHandle>> GEDSFileHandle::relocate() {
  return absl::UnavailableError("Relocating is not supported for this file handle type!");
}

void GEDSFileHandle::notifyUnused() { LOG_DEBUG("The file ", identifier, " is unused."); }

std::chrono::system_clock::time_point GEDSFileHandle::lastOpened() const { return _lastOpened; }
std::chrono::system_clock::time_point GEDSFileHandle::lastReleased() const { return _lastReleased; }

size_t GEDSFileHandle::roundToNearestMultiple(size_t number, size_t factor) const {
  return ((number + factor - 1) / factor) * factor;
}

std::optional<std::string> GEDSFileHandle::metadata() const {
  auto lock = lockFile();
  return _metadata;
}

absl::Status GEDSFileHandle::setMetadata(std::optional<std::string> /* unused metadata */,
                                         bool /* unused seal */) {
  return absl::UnavailableError("Cannot set metadata on read-only file.");
}

absl::StatusOr<size_t> GEDSFileHandle::readBytes(uint8_t * /* unused bytes */,
                                                 size_t /* unused position */,
                                                 size_t /* unused length */) {
  return absl::UnavailableError("Read operation is not available.");
}

absl::StatusOr<int> GEDSFileHandle::rawFd() const {
  return absl::UnavailableError("rawFDs are not supported for this FileHandle type!");
}

absl::StatusOr<uint8_t *> GEDSFileHandle::rawPtr() {
  return absl::UnavailableError("rawPtrs are not supported for this FileHandle type!");
}

absl::Status GEDSFileHandle::writeBytes(const uint8_t * /* unused bytes */,
                                        size_t /* unused position */, size_t /* unused length */) {
  return absl::UnavailableError("Write operation is not available.");
}

absl::Status GEDSFileHandle::write(std::istream & /* stream */, size_t /* position */,
                                   std::optional<size_t> /* lengthOptional */) {
  return absl::UnavailableError("Write is not available.");
}

absl::Status GEDSFileHandle::truncate(size_t /*targetSize*/) {
  return absl::UnavailableError("Truncate is not available.");
}

absl::Status GEDSFileHandle::download(std::shared_ptr<GEDSFileHandle> destination) {
  size_t pos = 0;
  const auto totalSize = size();
  if (!totalSize.ok()) {
    return totalSize.status();
  }
  do {
    auto length = std::min(_gedsService->config().cacheBlockSize, *totalSize - pos);
    auto count = downloadRange(destination, pos, length, pos);
    if (!count.ok()) {
      return count.status();
    }
    pos += *count;
  } while (pos < *totalSize);
  return absl::OkStatus();
}

absl::StatusOr<size_t> GEDSFileHandle::downloadRange(std::shared_ptr<GEDSFileHandle> destination,
                                                     size_t srcPosition, size_t length,
                                                     size_t destPosition) {
  if (this == destination.get()) {
    return absl::InvalidArgumentError("Source and target are the same!");
  }
  size_t count = 0;
  try {
    std::vector<uint8_t> buffer(_gedsService->config().cacheBlockSize, 0);
    do {
      auto rcount =
          readBytes(&buffer[0], srcPosition + count, std::min(length - count, buffer.size()));
      if (!rcount.ok()) {
        return rcount.status();
      }
      if (*rcount == 0) {
        break;
      }
      auto writeStatus = destination->writeBytes(&buffer[0], destPosition + count, *rcount);
      if (!writeStatus.ok()) {
        return writeStatus;
      }
      count += *rcount;
    } while (count < length);
  } catch (const std::runtime_error &e) {
    return absl::UnknownError(e.what());
  }
  return count;
}

absl::Status GEDSFileHandle::seal() {
  return absl::UnavailableError("Seal operation is not available.");
}

absl::StatusOr<GEDSFile> GEDSFileHandle::open() {
  // Avoid race-conditions when marking files as unused.
  auto lock = lockFile();
  return GEDSFile(_gedsService, shared_from_this());
}
