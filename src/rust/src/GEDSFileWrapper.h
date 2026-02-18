/**
* Copyright 2022- IBM Inc. All rights reserved
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef GEDSFILEWRAPPER_H
#define GEDSFILEWRAPPER_H

#include <memory>
#include "rust/cxx.h"

class GEDSFile;

namespace shared {
  struct Status;
  struct StatusOrUsize;
}

namespace geds_rs {
  class GEDSFileWrapper {
    std::unique_ptr<GEDSFile> gedsFile;

  public:
    explicit GEDSFileWrapper(const GEDSFile &filePtr);

    [[nodiscard]] size_t size() const;

    [[nodiscard]] rust::String identifier() const;

    [[nodiscard]] bool is_writeable() const;

    [[nodiscard]] rust::String metadata() const;

    [[nodiscard]] shared::Status seal() const;

    [[nodiscard]] shared::Status truncate(size_t size) const;

    [[nodiscard]] shared::Status set_metadata(const rust::Str metadata, bool seal) const;

    [[nodiscard]] shared::StatusOrUsize read(rust::Vec<uint8_t> &buffer, size_t position, size_t length) const;

    [[nodiscard]] shared::Status write(const rust::Vec<uint8_t> &buffer, size_t position, size_t length) const;
  };
}

#endif //GEDSFILEWRAPPER_H
