/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "com_ibm_geds_GEDSFile.h"

#include <chrono>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <jni.h>
#include <string>
#include <vector>

#include "GEDSFile.h"
#include "JavaError.h"
#include "Logging.h"
#include "Statistics.h"

// NOLINTBEGIN(modernize-use-trailing-return-type)

JNIEXPORT void JNICALL Java_com_ibm_geds_GEDSFile_closeNative(JNIEnv * /* unused env */, jobject,
                                                              jlong nativePtr) {
  static auto timer = geds::Statistics::createNanoSecondHistogram("Java GEDSFile: close");
  auto timerBegin = std::chrono::high_resolution_clock::now();

  if (nativePtr == 0) {
    LOG_DEBUG("Double close on GEDSFile.");
    return;
  }
  auto *file = reinterpret_cast<GEDSFile *>(nativePtr); // NOLINT
  delete file;                                          // NOLINT

  *timer += std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now() - timerBegin)
                .count();
}

// NOLINTNEXTLINE
JNIEXPORT jlong JNICALL Java_com_ibm_geds_GEDSFile_sizeNative(JNIEnv *env, jobject,
                                                              jlong nativePtr) {
  static auto timer = geds::Statistics::createNanoSecondHistogram("Java GEDSFile: size");
  auto timerBegin = std::chrono::high_resolution_clock::now();

  if (nativePtr == 0) {
    return throwNullPointerException(env, "The pointer representation is NULL!");
  }
  auto *file = reinterpret_cast<GEDSFile *>(nativePtr); // NOLINT
  auto size = file->size();
  if (size > LONG_MAX) {
    return throwRuntimeException(env, "Unable to represent the file size for " + file->bucket() +
                                          "/" + file->key() +
                                          " as a long. File size: " + std::to_string(size));
  }
  auto result = (long)file->size();
  *timer += std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now() - timerBegin)
                .count();
  return result;
}

// NOLINTNEXTLINE
JNIEXPORT jint JNICALL Java_com_ibm_geds_GEDSFile_readNative__JJ_3BII(JNIEnv *env, jobject,
                                                                      jlong nativePtr,
                                                                      jlong position,
                                                                      jbyteArray jBuffer,
                                                                      jint offset, jint length) {
  static auto counter = geds::Statistics::createIOHistogram("Java GEDSFile: bytes read");
  static auto timer = geds::Statistics::createNanoSecondHistogram("Java GEDSFile: read");
  auto timerBegin = std::chrono::high_resolution_clock::now();

  if (nativePtr == 0) {
    return throwNullPointerException(env, "The pointer representation is NULL!");
  }
  auto *file = reinterpret_cast<GEDSFile *>(nativePtr); // NOLINT

  // See:
  // https://docs.oracle.com/en/java/javase/11/docs/specs/jni/functions.html#getprimitivearraycritical-releaseprimitivearraycritical
  // NOLINTNEXTLINE
  auto buffer = reinterpret_cast<jbyte *>(env->GetPrimitiveArrayCritical(jBuffer, nullptr));
  if (buffer == nullptr) {
    // Operation failed.
    throwRuntimeException(env, "Unable to obtain primitive array.");
    return -1;
  }
  auto readStatus = file->read(&buffer[offset], position, length); // NOLINT
  // 0:  copy back the content and free the elems buffer (ignored if not a copy).
  env->ReleasePrimitiveArrayCritical(jBuffer, buffer, 0);
  if (!readStatus.ok()) {
    throwIOException(env, readStatus.status().message());
    return 0;
  }

  *counter += *readStatus;
  *timer += std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now() - timerBegin)
                .count();
  return *readStatus; // NOLINT
}

JNIEXPORT void JNICALL Java_com_ibm_geds_GEDSFile_writeNative__JJ_3BII(JNIEnv *env, jobject,
                                                                       jlong nativePtr,
                                                                       jlong position,
                                                                       jbyteArray jBuffer,
                                                                       jint offset, jint length) {
  static auto counter = geds::Statistics::createIOHistogram("Java GEDSFile: bytes written");
  static auto timer = geds::Statistics::createNanoSecondHistogram("Java GEDSFile: write");
  auto timerBegin = std::chrono::high_resolution_clock::now();

  if (nativePtr == 0) {
    throwNullPointerException(env, "The pointer representation is NULL!");
    return;
  }
  auto *file = reinterpret_cast<GEDSFile *>(nativePtr); // NOLINT
  // See:
  // https://docs.oracle.com/en/java/javase/11/docs/specs/jni/functions.html#getprimitivearraycritical-releaseprimitivearraycritical
  auto buffer =
      reinterpret_cast<jbyte *>(env->GetPrimitiveArrayCritical(jBuffer, nullptr)); // NOLINT
  if (buffer == nullptr) {
    // Operation failed.
    throwRuntimeException(env, "Unable to obtain primitive array.");
    return;
  }
  auto writeStatus = file->write(&buffer[offset], position, length); // NOLINT
  // JNI_ABORT: free the buffer without copying back the possible changes
  env->ReleasePrimitiveArrayCritical(jBuffer, buffer, JNI_ABORT);
  if (!writeStatus.ok()) {
    throwIOException(env, writeStatus.message());
  }

  *counter += length;
  *timer += std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now() - timerBegin)
                .count();
}

JNIEXPORT jint JNICALL Java_com_ibm_geds_GEDSFile_readNative__JJJII(JNIEnv *env, jobject,
                                                                    jlong nativePtr, jlong position,
                                                                    jlong buffer, jint offset,
                                                                    jint length) {
  static auto counter = geds::Statistics::createIOHistogram("Java GEDSFile: bytes read");
  static auto timer = geds::Statistics::createNanoSecondHistogram("Java GEDSFile: read");
  auto timerBegin = std::chrono::high_resolution_clock::now();

  if (nativePtr == 0) {
    return throwNullPointerException(env, "The pointer representation is NULL!");
  }
  if (buffer == 0) {
    return throwNullPointerException(env, "The buffer is NULL!");
  }
  auto buf = reinterpret_cast<uint8_t *>(buffer);
  auto file = reinterpret_cast<GEDSFile *>(nativePtr);          // NOLINT
  auto readStatus = file->read(&buf[offset], position, length); // NOLINT
  if (!readStatus.ok()) {
    throwIOException(env, readStatus.status().message());
    return 0;
  }

  *counter += *readStatus;
  *timer += std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now() - timerBegin)
                .count();
  return *readStatus; // NOLINT
}

JNIEXPORT void JNICALL Java_com_ibm_geds_GEDSFile_writeNative__JJJII(JNIEnv *env, jobject,
                                                                     jlong nativePtr,
                                                                     jlong position, jlong buffer,
                                                                     jint offset, jint length) {
  static auto counter = geds::Statistics::createIOHistogram("Java GEDSFile: bytes written");
  static auto timer = geds::Statistics::createNanoSecondHistogram("Java GEDSFile: write");
  auto timerBegin = std::chrono::high_resolution_clock::now();

  if (nativePtr == 0) {
    throwNullPointerException(env, "The pointer representation is NULL!");
    return;
  }
  if (buffer == 0) {
    throwNullPointerException(env, "The buffer is NULL!");
    return;
  }
  auto file = reinterpret_cast<GEDSFile *>(nativePtr);            // NOLINT
  auto buf = reinterpret_cast<const uint8_t *>(buffer);           // NOLINT
  auto writeStatus = file->write(&buf[offset], position, length); // NOLINT
  if (!writeStatus.ok()) {
    throwIOException(env, writeStatus.message());
  }
  *counter += length;
  *timer += std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now() - timerBegin)
                .count();
}

JNIEXPORT void JNICALL Java_com_ibm_geds_GEDSFile_sealNative(JNIEnv *env, jobject,
                                                             jlong nativePtr) {
  static auto timer = geds::Statistics::createNanoSecondHistogram("Java GEDSFile: seal");
  auto timerBegin = std::chrono::high_resolution_clock::now();

  if (nativePtr == 0) {
    throwNullPointerException(env, "The pointer representation is NULL!");
    return;
  }
  auto file = reinterpret_cast<GEDSFile *>(nativePtr); // NOLINT
  auto status = file->seal();
  if (!status.ok()) {
    throwIOException(env, status.message());
  }
  *timer += std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now() - timerBegin)
                .count();
}

JNIEXPORT void JNICALL Java_com_ibm_geds_GEDSFile_truncateNative(JNIEnv *env, jobject,
                                                                 jlong nativePtr,
                                                                 jlong targetSize) {
  static auto timer = geds::Statistics::createNanoSecondHistogram("Java GEDSFile: truncate");
  auto timerBegin = std::chrono::high_resolution_clock::now();

  if (nativePtr == 0) {
    throwNullPointerException(env, "The pointer representation is NULL!");
    return;
  }
  auto file = reinterpret_cast<GEDSFile *>(nativePtr); // NOLINT
  auto status = file->truncate(targetSize);
  if (!status.ok()) {
    throwIOException(env, status.message());
  }

  *timer += std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now() - timerBegin)
                .count();
}

JNIEXPORT jboolean JNICALL Java_com_ibm_geds_GEDSFile_isWritableNative(JNIEnv *env, jobject,
                                                                       jlong nativePtr) {
  static auto timer = geds::Statistics::createNanoSecondHistogram("Java GEDSFile: isWriteable");
  auto timerBegin = std::chrono::high_resolution_clock::now();

  if (nativePtr == 0) {
    throwNullPointerException(env, "The pointer representation is NULL!");
    return false;
  }
  auto file = reinterpret_cast<GEDSFile *>(nativePtr); // NOLINT
  auto result = file->isWriteable();

  *timer += std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now() - timerBegin)
                .count();
  return result;
}

// NOLINTEND(modernize-use-trailing-return-type)
