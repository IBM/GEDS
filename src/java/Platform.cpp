/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#include <jni.h>

/**
 * Make sure that we can expose a pointer to Java.
 */
static_assert(sizeof(jlong) == sizeof(void *));
