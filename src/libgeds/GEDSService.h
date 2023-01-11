/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#ifndef GEDS_GEDSSERVICE_H
#define GEDS_GEDSSERVICE_H

#include "GEDSProtocol.h"

class GEDSService {
protected:
  const geds::Protocol _protocol;

public:
  GEDSService(geds::Protocol protocol);
  virtual ~GEDSService() = default;

  virtual std::string remoteObjectUrl(const std::string &bucket, const std::string &key) = 0;
};

#endif // GEDS_GEDSSERVICE_H
