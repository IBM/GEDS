/**
* Copyright 2022- IBM Inc. All rights reserved
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef GEDSWRAPPER_H
#define GEDSWRAPPER_H

#include <memory>
#include "rust/cxx.h"

class GEDSFile;
class GEDS;

namespace shared {
  struct GEDSConfig;
  struct GEDSFileStatus;
  struct Status;
  struct StatusOrGEDSFileWrapper;
  struct StatusOrGEDSFileStatus;
  struct StatusOrVecGEDSFileStatus;
}

namespace geds_rs {
  class GEDSWrapper {
  protected:
    std::shared_ptr<GEDS> gedsPtr;

  public:
    explicit GEDSWrapper(const shared::GEDSConfig &);

    [[nodiscard]] shared::Status start() const;

    [[nodiscard]] shared::Status stop() const;

    [[nodiscard]] shared::StatusOrGEDSFileWrapper create(const rust::Str bucket, const rust::Str key,
                                                         bool overwrite) const;

    [[nodiscard]] shared::Status create_bucket(const rust::Str bucket) const;

    [[nodiscard]] shared::Status mkdirs(const rust::Str bucket, const rust::Str path) const;

    [[nodiscard]] shared::StatusOrVecGEDSFileStatus list(const rust::Str bucket, const rust::Str key) const;

    [[nodiscard]] shared::StatusOrVecGEDSFileStatus list_folder(const rust::Str bucket,
                                                                const rust::Str prefix) const;

    [[nodiscard]] shared::StatusOrGEDSFileStatus status(const rust::Str bucket, const rust::Str key) const;

    [[nodiscard]] shared::StatusOrGEDSFileWrapper open(const rust::Str bucket, const rust::Str key) const;

    [[nodiscard]] shared::Status delete_object(const rust::Str bucket, const rust::Str key) const;

    [[nodiscard]] shared::Status delete_object_prefix(const rust::Str bucket, const rust::Str prefix) const;

    [[nodiscard]] shared::Status rename(const rust::Str src_bucket, const rust::Str src_key,
                                        const rust::Str dest_bucket, const rust::Str dest_key) const;

    [[nodiscard]] shared::Status rename_prefix(const rust::Str src_bucket, const rust::Str src_prefix,
                                               const rust::Str dest_bucket, const rust::Str dest_prefix) const;

    [[nodiscard]] shared::Status copy(const rust::Str src_bucket, const rust::Str src_key,
                                      const rust::Str dest_bucket, const rust::Str dest_key) const;

    [[nodiscard]] shared::Status copy_prefix(const rust::Str src_bucket, const rust::Str src_prefix,
                                             const rust::Str dest_bucket, const rust::Str dest_prefix) const;

    [[nodiscard]] rust::String local_path(const rust::Str bucket, const rust::Str key) const;

    [[nodiscard]] shared::Status register_object_store_config(const rust::Str bucket,
                                                              const rust::Str endpoint_url,
                                                              const rust::Str access_key,
                                                              const rust::Str secret_key) const;

    [[nodiscard]] shared::Status sync_object_store_configs() const;

    void relocate(bool force) const;

    [[nodiscard]] shared::Status subscribe(const rust::Str bucket, const rust::Str key, const int &subscription_type) const;

  };

  std::unique_ptr<GEDSWrapper> new_wrapper(const shared::GEDSConfig &sharedConfig);
}

#endif //GEDSWRAPPER_H
