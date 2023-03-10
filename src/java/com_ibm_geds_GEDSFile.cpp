/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "com_ibm_geds_GEDSFile.h"

#include <climits>
#include <cstddef>
#include <cstdint>
#include <jni.h>
#include <string>

#include "GEDSFile.h"
#include "JavaError.h"
#include "Logging.h"
#include "Statistics.h"

// NOLINTBEGIN(modernize-use-trailing-return-type)

JNIEXPORT void JNICALL Java_com_ibm_geds_GEDSFile_closeNative(JNIEnv * /* unused env */, jobject,
                                                              jlong nativePtr) {
  static auto counter = geds::Statistics::counter("Java GEDSFile: close");

  if (nativePtr == 0) {
    LOG_DEBUG("Double close on GEDSFile.");
    return;
  }
  auto *file = reinterpret_cast<GEDSFile *>(nativePtr); // NOLINT
  delete file;                                          // NOLINT
  *counter += 1;
}

// NOLINTNEXTLINE
JNIEXPORT jlong JNICALL Java_com_ibm_geds_GEDSFile_sizeNative(JNIEnv *env, jobject,
                                                              jlong nativePtr) {
  static auto counter = geds::Statistics::counter("Java GEDSFile: size");

  if (nativePtr == 0) {
    return throwNullPointerException(env, "The pointer representation is NULL!");
  }
  auto *file = reinterpret_cast<GEDSFile *>(nativePtr); // NOLINT
  *counter += 1;
  auto size = file->size();
  if (size > LONG_MAX) {
    return throwRuntimeException(env, "Unable to represent the file size for " + file->bucket() +
                                          "/" + file->key() +
                                          " as a long. File size: " + std::to_string(size));
  }
  return (long)file->size();
}

// NOLINTNEXTLINE
JNIEXPORT jint JNICALL Java_com_ibm_geds_GEDSFile_readNative__JJ_3BII(JNIEnv *env, jobject,
                                                                      jlong nativePtr,
                                                                      jlong position,
                                                                      jbyteArray jBuffer,
                                                                      jint offset, jint length) {
  static auto counter = geds::Statistics::counter("Java GEDSFile: bytes read");

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
  return *readStatus; // NOLINT
}

JNIEXPORT void JNICALL Java_com_ibm_geds_GEDSFile_writeNative__JJ_3BII(JNIEnv *env, jobject,
                                                                       jlong nativePtr,
                                                                       jlong position,
                                                                       jbyteArray jBuffer,
                                                                       jint offset, jint length) {
  static auto counter = geds::Statistics::counter("Java GEDSFile: bytes written");

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
}

// NOLINTNEXTLINE
JNIEXPORT jint JNICALL Java_com_ibm_geds_GEDSFile_readNative__JJLjava_nio_ByteBuffer_2II(
    JNIEnv *env, jobject, jlong nativePtr, jlong position, jobject jBuffer, jint offset,
    jint length) {
  static auto counter = geds::Statistics::counter("Java GEDSFile: bytes read");

  if (nativePtr == 0) {
    return throwNullPointerException(env, "The pointer representation is NULL!");
  }
  auto file = reinterpret_cast<GEDSFile *>(nativePtr);           // NOLINT
  auto buffer = (uint8_t *)env->GetDirectBufferAddress(jBuffer); // NOLINT
#ifndef NDEBUG
  auto capacity = env->GetDirectBufferCapacity(jBuffer);
  if ((size_t)offset + (size_t)length > (size_t)capacity) {
    throwRuntimeException(env, "Offset + length are bigger than buffer capacity.");
    return -1;
  }
#endif
  auto readStatus = file->read(&buffer[offset], position, length); // NOLINT
  if (!readStatus.ok()) {
    throwIOException(env, readStatus.status().message());
    return 0;
  }
  *counter += *readStatus;
  return *readStatus; // NOLINT
}

JNIEXPORT void JNICALL Java_com_ibm_geds_GEDSFile_writeNative__JJLjava_nio_ByteBuffer_2II(
    JNIEnv *env, jobject, jlong nativePtr, jlong position, jobject jBuffer, jint offset,
    jint length) {
  static auto counter = geds::Statistics::counter("Java GEDSFile: bytes written");

  if (nativePtr == 0) {
    throwNullPointerException(env, "The pointer representation is NULL!");
    return;
  }
  auto file = reinterpret_cast<GEDSFile *>(nativePtr);                                   // NOLINT
  auto buffer = reinterpret_cast<const uint8_t *>(env->GetDirectBufferAddress(jBuffer)); // NOLINT
#ifndef NDEBUG
  auto capacity = env->GetDirectBufferCapacity(jBuffer);
  if ((size_t)offset + (size_t)length > (size_t)capacity) {
    throwRuntimeException(env, "Offset + length are bigger than buffer capacity.");
    return;
  }
#endif
  auto writeStatus = file->write(&buffer[offset], position, length); // NOLINT
  if (!writeStatus.ok()) {
    throwIOException(env, writeStatus.message());
  }
  *counter += length;
}

JNIEXPORT void JNICALL Java_com_ibm_geds_GEDSFile_sealNative(JNIEnv *env, jobject,
                                                             jlong nativePtr) {
  static auto counter = geds::Statistics::counter("Java GEDSFile: files sealed");

  if (nativePtr == 0) {
    throwNullPointerException(env, "The pointer representation is NULL!");
    return;
  }
  auto file = reinterpret_cast<GEDSFile *>(nativePtr); // NOLINT
  auto status = file->seal();
  if (!status.ok()) {
    throwIOException(env, status.message());
  }
}

JNIEXPORT void JNICALL Java_com_ibm_geds_GEDSFile_truncateNative(JNIEnv *env, jobject,
                                                                 jlong nativePtr,
                                                                 jlong targetSize) {
  static auto counter = geds::Statistics::counter("Java GEDSFile: files truncated");

  if (nativePtr == 0) {
    throwNullPointerException(env, "The pointer representation is NULL!");
    return;
  }
  auto file = reinterpret_cast<GEDSFile *>(nativePtr); // NOLINT
  auto status = file->truncate(targetSize);
  if (!status.ok()) {
    throwIOException(env, status.message());
  }
}

JNIEXPORT jboolean JNICALL Java_com_ibm_geds_GEDSFile_isWritableNative(JNIEnv *env, jobject,
                                                                       jlong nativePtr) {
  static auto counter = geds::Statistics::counter("Java GEDSFile: isWriteable");

  if (nativePtr == 0) {
    throwNullPointerException(env, "The pointer representation is NULL!");
    return false;
  }
  auto file = reinterpret_cast<GEDSFile *>(nativePtr); // NOLINT
  return file->isWriteable();
}

// NOLINTEND(modernize-use-trailing-return-type)
