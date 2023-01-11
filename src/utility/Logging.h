/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#include <iostream>

#include "LogNullSink.h"

#ifndef GEDS_LOGGING_H
#define GEDS_LOGGING_H

#define LOG_LINE __func__ << " (" << __FILE__ << ": " << __LINE__ << "): " // NOLINT
#define LOG_ERROR std::clog << "ERROR " << LOG_LINE                        // NOLINT
#define LOG_INFO std::clog << "INFO  " << LOG_LINE                         // NOLINT
#define LOG_WARNING std::clog << "WARN  " << LOG_LINE                      // NOLINT

#if defined(NDEBUG)
#define LOG_DEBUG ::utility::NULL_SINK << "DEBUG " << LOG_LINE // NOLINT
#else
#define LOG_DEBUG std::clog << "DEBUG " << LOG_LINE // NOLINT
#endif

#endif
