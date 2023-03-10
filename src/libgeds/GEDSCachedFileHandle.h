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
  mutable bool _isValid{true};

  std::shared_ptr<GEDSFileHandle> _remoteFileHandle;
  std::shared_ptr<GEDSFile> _remoteFile;

  size_t _remoteSize;
  size_t _blockSize;

  std::vector<std::shared_ptr<GEDSFile>> _blocks;
  std::vector<std::mutex> _blockMutex;

  std::shared_ptr<geds::StatisticsCounter> _readStatistics =
      geds::Statistics::counter("GEDSCachedFileHandle: bytes read");
  std::shared_ptr<geds::StatisticsCounter> _cacheSize =
      geds::Statistics::counter("GEDSCachedFileHandle: local cache size");
  std::shared_ptr<geds::StatisticsCounter> _numCachedBlocks =
      geds::Statistics::counter("GEDSCachedFileHandle: number of locally cached blocks");
  std::shared_ptr<geds::StatisticsCounter> _numPurgedBlocks =
      geds::Statistics::counter("GEDSCachedFileHandle: number of purged blocks");
  // private:
public:
  GEDSCachedFileHandle(std::shared_ptr<GEDS> gedsService, std::string bucketArg, std::string keyArg,
                       std::shared_ptr<GEDSFileHandle> remoteFile);

public:
  static const std::string CacheBlockMarker;

  template <class TRemote>
  [[nodiscard]] static absl::StatusOr<std::shared_ptr<GEDSFileHandle>>
  factory(std::shared_ptr<GEDS> gedsService, const std::string &bucket, const std::string &key) {
    auto remoteFH = TRemote::factory(gedsService, bucket, key);
    if (!remoteFH.ok()) {
      return remoteFH.status();
    }
    return std::shared_ptr<GEDSFileHandle>(
        new GEDSCachedFileHandle(gedsService, bucket, key, *remoteFH));
  }

  template <class TRemote>
  [[nodiscard]] static absl::StatusOr<std::shared_ptr<GEDSFileHandle>>
  factory(std::shared_ptr<GEDS> gedsService, const geds::Object &object) {
    auto remoteFH = TRemote::factory(gedsService, object);
    if (!remoteFH.ok()) {
      return remoteFH.status();
    }
    return std::shared_ptr<GEDSFileHandle>(
        new GEDSCachedFileHandle(gedsService, object.id.bucket, object.id.key, *remoteFH));
  }

  GEDSCachedFileHandle() = delete;
  GEDSCachedFileHandle(const GEDSCachedFileHandle &) = delete;
  GEDSCachedFileHandle(GEDSCachedFileHandle &&) = delete;
  ~GEDSCachedFileHandle() override = default;
  GEDSCachedFileHandle &operator=(const GEDSCachedFileHandle &) = delete;
  GEDSCachedFileHandle &operator=(GEDSCachedFileHandle &&) = delete;

  absl::StatusOr<size_t> size() const override;

  absl::StatusOr<size_t> readBytes(uint8_t *bytes, size_t position, size_t length) override;

  absl::Status seal() override;

  bool isValid() const override;
};

#endif
