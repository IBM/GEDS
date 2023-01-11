/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#pragma once

#include <ios>
#include <iostream>

#include <boost/interprocess/streams/bufferstream.hpp>

namespace geds::utility {

/**
 * @brief A zero-copy Bytestream that uses an array as a backing-store.
 * Based on boost::bufferstream.
 *
 * @tparam iostream type.
 */
template <class S> class ByteStream : public S {

public:
  ByteStream(uint8_t *array, size_t length,
             std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
      : S(reinterpret_cast<char *>(array), length, mode) {}

  /**
   * @brief Construct a new Byte Stream object
   *
   * @param array pointer of the backing store.
   * @param length length of the backing store. `setg` will be set to 0.
   */
  ByteStream(std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out) : S(mode) {}
};

using ByteOStream = ByteStream<boost::interprocess::obufferstream>;
using ByteIStream = ByteStream<boost::interprocess::ibufferstream>;
using ByteIOStream = ByteStream<boost::interprocess::bufferstream>;

} // namespace geds::utility
