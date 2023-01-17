/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "JavaError.h"

#include <cstring>

// NOLINTNEXTLINE
jint throwJavaException(JNIEnv *env, const std::string &className,
                        const std::string_view &message) {
  // Use a static buffer to avoid memory leaks and ensure that the error message survives.
  auto javaClass = (*env).FindClass(className.c_str());
  auto messageStr = std::string{message};
  return (*env).ThrowNew(javaClass, messageStr.c_str());
}
