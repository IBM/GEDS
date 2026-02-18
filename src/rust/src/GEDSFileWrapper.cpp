/**
* Copyright 2022- IBM Inc. All rights reserved
* SPDX-License-Identifier: Apache-2.0
*/

#include "GEDSFileWrapper.h"

#include "GEDSFile.h"
#include "Logging.h"
#include "geds_rs/src/lib.rs.h"

namespace geds_rs {
  GEDSFileWrapper::GEDSFileWrapper(const GEDSFile &filePtr) {
    gedsFile = std::make_unique<GEDSFile>(filePtr);
  }

  size_t GEDSFileWrapper::size() const {
    return gedsFile->size();
  }

  rust::String GEDSFileWrapper::identifier() const {
    return rust::String(gedsFile->identifier());
  }

  bool GEDSFileWrapper::is_writeable() const {
    return gedsFile->isWriteable();
  }

  rust::String GEDSFileWrapper::metadata() const {
    return rust::String(gedsFile->metadata().value());
  }

  template<typename T>
  std::vector<T> toStdVector(rust::Vec<T> v) {
    std::vector<T> stdv;

    return stdv;
  }

  shared::Status GEDSFileWrapper::seal() const {
    const auto status = gedsFile->seal();
    return {.message = rust::string(status.ToString()), .ok = status.ok()};
  }

  shared::Status GEDSFileWrapper::truncate(const size_t size) const {
    const auto status = gedsFile->truncate(size);
    return shared::Status{.message = status.ToString(), .ok = status.ok()};
  }

  shared::Status GEDSFileWrapper::set_metadata(const rust::Str metadata, const bool seal) const {
    auto cppMetadata = std::string(metadata);
    const auto status = gedsFile->setMetadata(cppMetadata, seal);
    return shared::Status{.message = status.ToString(), .ok = status.ok()};
  }

  shared::StatusOrUsize GEDSFileWrapper::read(rust::Vec<uint8_t> &buffer, const size_t position,
                                              const size_t length) const {
    // TODO: can we get rid of the extra copy? Working with buffer.data() as in write() does not work
    std::vector<uint8_t> stdv;
    const auto status = gedsFile->read(stdv, position, length);
    std::copy(stdv.begin(), stdv.end(), std::back_inserter(buffer));
    
    return shared::StatusOrUsize{
      .status = {.message = status.status().ToString(), .ok = status.ok()},
      .value = status.value_or(0)
    };
  }

  shared::Status GEDSFileWrapper::write(const rust::Vec<uint8_t> &buffer, const size_t position,
                                        const size_t length) const {
    const auto status = gedsFile->write(buffer.data(), position, length);
    return shared::Status{.message = status.ToString(), .ok = status.ok()};
  }
}
