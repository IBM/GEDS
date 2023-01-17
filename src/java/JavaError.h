/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GEDS_JAVAERROR_H
#define GEDS_JAVAERROR_H

#include <jni.h>

#include <string>

// NOLINTNEXTLINE
jint throwJavaException(JNIEnv *env, const std::string &className, const std::string_view &message);

template <typename T> auto throwRuntimeException(JNIEnv *env, const T &message) {
  return throwJavaException(env, "java/lang/RuntimeException", message);
}

template <typename T> auto throwNullPointerException(JNIEnv *env, const T &message) {
  return throwJavaException(env, "java/lang/NullPointerException", message);
}

template <typename T> auto throwIOException(JNIEnv *env, const T &message) {
  return throwJavaException(env, "java/io/IOException", message);
}

template <typename T> auto throwEOFException(JNIEnv *env, const T &message) {
  return throwJavaException(env, "java/io/EOFException", message);
}

template <typename T> auto throwFileNotFoundException(JNIEnv *env, const T &message) {
  return throwJavaException(env, "java/io/FileNotFoundException", message);
}

#endif // GEDS_JAVAERROR_H
