/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
#include <future>
#include <memory>
#include <mutex>
#include <ostream>
#include <stdexcept>
#include <vector>

#include "GEDSCachedFileHandle.h"
#include "GEDSFile.h"
#include "GEDSFileHandle.h"
#include "Logging.h"
#include "Statistics.h"

const std::string GEDSCachedFileHandle::CacheBlockMarker = {"_$cachedblock$/"};

GEDSCachedFileHandle::GEDSCachedFileHandle(std::shared_ptr<GEDS> gedsService, std::string bucketArg,
                                           std::string keyArg,
                                           std::shared_ptr<GEDSFileHandle> remoteFileHandle)
    : GEDSFileHandle(gedsService, std::move(bucketArg), std::move(keyArg)),
      _remoteFileHandle(remoteFileHandle), _blockSize(gedsService->blockSize) {
  static auto counter = geds::Statistics::createCounter("GEDSCachedFileHandle: count");
  *counter += 1;

  auto fileOpenStatus = _remoteFileHandle->open();
  if (!fileOpenStatus.ok()) {
    auto message = "Unable to open GEDSFile from filehandle: " +
                   std::string{fileOpenStatus.status().message()};
    LOG_ERROR(message);
    throw std::runtime_error(message);
  }
  _remoteFile = std::make_shared<GEDSFile>(std::move(*fileOpenStatus));

  _remoteSize = _remoteFile->size();
  _blocks = std::vector<std::shared_ptr<GEDSFile>>(_remoteSize / _blockSize + 1, nullptr);
  _blockMutex = std::vector<std::mutex>(_remoteSize / _blockSize + 1);
}

absl::StatusOr<size_t> GEDSCachedFileHandle::size() const { return _remoteSize; }

absl::StatusOr<size_t> GEDSCachedFileHandle::readBytes(uint8_t *bytes, size_t position,
                                                       size_t length) {

  if (position >= _remoteSize || length == 0) {
    return 0;
  }
  length = std::min(length, _remoteSize - position);

  auto computeBlock = [&](size_t pos) { return pos / _blockSize; };
  auto openBlock = [&](size_t idx) -> absl::StatusOr<GEDSFile> {
    if (_blocks[idx].get() != nullptr) {
      return *_blocks[idx];
    }
    auto cacheKey = CacheBlockMarker + key + "_" + std::to_string(idx);
    auto lock = std::lock_guard(_blockMutex[idx]);
    auto exists = _gedsService->open(bucket, cacheKey);
    if (exists.ok()) {
      _blocks[idx] = std::make_shared<GEDSFile>(std::move(*exists));
      return *_blocks[idx];
    }

    auto newFile = _gedsService->createAsFileHandle(bucket, cacheKey);
    if (!newFile.ok()) {
      return newFile.status();
    }
    auto copyStatus = _remoteFileHandle->downloadRange(*newFile, _blockSize * idx, _blockSize, 0);
    if (!copyStatus.ok()) {
      return copyStatus.status();
    }
    *_cacheSize += *copyStatus;

    auto f = (*newFile)->open();
    if (!f.ok()) {
      return f.status();
    }
    *_numCachedBlocks += 1;
    _blocks[idx] = std::make_shared<GEDSFile>(std::move(*(*newFile)->open()));
    auto sealStatus = (*newFile)->seal();
    if (!sealStatus.ok()) {
      LOG_ERROR("Unable to seal block ", identifier, " with ", idx, ": ", sealStatus.message());
    }
    return *_blocks[idx];
  };

  auto purgeBlock = [&](size_t idx, GEDSFile &file) {
    LOG_INFO("PurgeBlock ", file.identifier());
    auto lock = std::lock_guard(_blockMutex[idx]);
    if (_blocks[idx].get() == nullptr) {
      return;
    }
    const GEDSFileHandle *e = _blocks[idx]->fileHandle().get();
    const GEDSFileHandle *t = file.fileHandle().get();
    if (e == t) {
      *_numPurgedBlocks += 1;
      _blocks[idx] = nullptr;
      LOG_INFO("About to purge block", file.identifier());
      (void)_gedsService->deleteObject(file.bucket(), file.key());
      LOG_INFO("Purged block ", file.identifier());
    }
  };

  auto startBlock = computeBlock(position);
  auto endBlock = computeBlock(position + length);

  const size_t MAX_RETRIES = 1;
  size_t count = 0;
  for (size_t idx = startBlock; idx <= endBlock; idx++) {
    size_t retryCount = 0;
    while (true) {
      auto fileBlock = openBlock(idx);
      if (!fileBlock.ok()) {
        return fileBlock.status();
      }
      auto file = *fileBlock;
      absl::StatusOr<size_t> copyCount;
      size_t expectedCount = std::min(length - count, _blockSize);
      copyCount = file.read(bytes + count,
                            (position + count) % _blockSize, //
                            expectedCount);
      if (copyCount.ok()) {
        *_readStatistics += *copyCount;
        count += *copyCount;
        break;
      }
      if (retryCount >= MAX_RETRIES) {
        return copyCount.status();
      }
      LOG_INFO("Unable to download block from ", identifier,
               ". Reason: ", copyCount.status().message(), ". Retrying");
      // Purge block and retry.
      purgeBlock(idx, file);
      retryCount++;
    }
  }
  return count;
}

absl::Status GEDSCachedFileHandle::seal() { return _remoteFileHandle->seal(); }

bool GEDSCachedFileHandle::isValid() const { return _isValid; };
