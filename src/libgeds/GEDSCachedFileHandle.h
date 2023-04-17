/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GEDS_CACHED_FILE_HANDLE_H
#define GEDS_CACHED_FILE_HANDLE_H

#include <memory>
#include <mutex>
#include <optional>
#include <vector>

#include "GEDSFile.h"
#include "MMAPFile.h"
#include "Object.h"

#include "GEDS.h"

class GEDSCachedFileHandle : public GEDSFileHandle {
  std::shared_ptr<GEDSFileHandle> _remoteFileHandle;
  std::shared_ptr<GEDSFile> _remoteFile;

  size_t _remoteSize;
  size_t _blockSize;

  std::vector<std::shared_ptr<GEDSFile>> _blocks;
  mutable std::vector<std::mutex> _blockMutex;

  std::shared_ptr<geds::StatisticsCounter> _readStatistics =
      geds::Statistics::createCounter("GEDSCachedFileHandle: bytes read");
  std::shared_ptr<geds::StatisticsCounter> _cacheSize =
      geds::Statistics::createCounter("GEDSCachedFileHandle: local cache size");
  std::shared_ptr<geds::StatisticsCounter> _numCachedBlocks =
      geds::Statistics::createCounter("GEDSCachedFileHandle: number of locally cached blocks");
  std::shared_ptr<geds::StatisticsCounter> _numPurgedBlocks =
      geds::Statistics::createCounter("GEDSCachedFileHandle: number of purged blocks");
  // private:
public:
  GEDSCachedFileHandle(std::shared_ptr<GEDS> gedsService, std::string bucketArg, std::string keyArg,
                       std::optional<std::string> metadataArg,
                       std::shared_ptr<GEDSFileHandle> remoteFile);

public:
  static const std::string CacheBlockMarker;

  template <class TRemote>
  [[nodiscard]] static absl::StatusOr<std::shared_ptr<GEDSFileHandle>>
  factory(std::shared_ptr<GEDS> gedsService, const std::string &bucket, const std::string &key,
          std::optional<std::string> metadataArg) {
    auto remoteFH = TRemote::factory(gedsService, bucket, key, metadataArg);
    if (!remoteFH.ok()) {
      return remoteFH.status();
    }
    return std::shared_ptr<GEDSFileHandle>(
        new GEDSCachedFileHandle(gedsService, bucket, key, std::move(metadataArg), *remoteFH));
  }

  template <class TRemote>
  [[nodiscard]] static absl::StatusOr<std::shared_ptr<GEDSFileHandle>>
  factory(std::shared_ptr<GEDS> gedsService, const geds::Object &object) {
    auto remoteFH = TRemote::factory(gedsService, object);
    if (!remoteFH.ok()) {
      return remoteFH.status();
    }
    return std::shared_ptr<GEDSFileHandle>(new GEDSCachedFileHandle(
        gedsService, object.id.bucket, object.id.key, object.info.metadata, *remoteFH));
  }

  GEDSCachedFileHandle() = delete;
  GEDSCachedFileHandle(const GEDSCachedFileHandle &) = delete;
  GEDSCachedFileHandle(GEDSCachedFileHandle &&) = delete;
  ~GEDSCachedFileHandle() override = default;
  GEDSCachedFileHandle &operator=(const GEDSCachedFileHandle &) = delete;
  GEDSCachedFileHandle &operator=(GEDSCachedFileHandle &&) = delete;

  bool isRelocatable() const override { return true; }
  absl::StatusOr<size_t> size() const override;
  size_t localStorageSize() const override;
  size_t localMemorySize() const override;

  absl::StatusOr<size_t> readBytes(uint8_t *bytes, size_t position, size_t length) override;

  absl::Status seal() override;

  absl::StatusOr<std::shared_ptr<GEDSFileHandle>> relocate() override;
};

#endif
