/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GEDS_REMOTE_FILE_HANDLE_H
#define GEDS_REMOTE_FILE_HANDLE_H

#include "FileTransferService.h"
#include "GEDSFileHandle.h"
#include "Object.h"
#include "Statistics.h"

class GEDSRemoteFileHandle : public GEDSFileHandle {
  std::shared_ptr<geds::FileTransferService> _fileTransferService;
  geds::ObjectInfo _info;

  std::shared_ptr<geds::StatisticsCounter> _statistics =
      geds::Statistics::createCounter("GEDSRemoteFileHandle: bytes read");

private:
  // Constructors are private to enable `shared_from_this`.
  GEDSRemoteFileHandle(std::shared_ptr<GEDS> gedsService, const geds::Object &object,
                       std::shared_ptr<geds::FileTransferService> fileTransferService);

public:
  GEDSRemoteFileHandle() = delete;
  ~GEDSRemoteFileHandle() override = default;

  [[nodiscard]] static absl::StatusOr<std::shared_ptr<GEDSFileHandle>>
  factory(std::shared_ptr<GEDS> gedsService, const geds::Object &object);

  const std::string path;

  absl::StatusOr<size_t> size() const override;

  absl::Status seal() override;

  absl::StatusOr<size_t> readBytes(uint8_t *bytes, size_t position, size_t length) override;
};

#endif
