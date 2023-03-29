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

/*
 * Class:     com_ibm_geds_GEDSFile
 * Method:    metadataNative
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ibm_geds_GEDSFile_metadataNative__J(JNIEnv *env, jobject,
                                                                       jlong nativePtr) {
  if (nativePtr == 0) {
    throwNullPointerException(env, "The pointer representation is NULL!");
    return nullptr;
  }
  auto *file = reinterpret_cast<GEDSFile *>(nativePtr); // NOLINT
  auto metadata = file->metadata();
  if (metadata->empty()) {
    return nullptr;
  }
  auto result = env->NewStringUTF(metadata.value().data());
  return result;
}

/*
 * Class:     com_ibm_geds_GEDSFile
 * Method:    metadataAsByteArrayNative
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_ibm_geds_GEDSFile_metadataAsByteArrayNative(JNIEnv *env,
                                                                                  jobject,
                                                                                  jlong nativePtr) {
  if (nativePtr == 0) {
    throwNullPointerException(env, "The pointer representation is NULL!");
    return nullptr;
  }
  auto *file = reinterpret_cast<GEDSFile *>(nativePtr); // NOLINT
  auto metadata = file->metadata();
  if (metadata->empty()) {
    return nullptr;
  }
  auto &resp = metadata.value();
  auto result = env->NewByteArray(resp.size());
  auto destbuffer = reinterpret_cast<char *>(env->GetPrimitiveArrayCritical(result, nullptr));
  if (destbuffer == nullptr) {
    // Operation failed.
    throwRuntimeException(env, "Unable to obtain primitive array.");
    return nullptr;
  }
  std::memcpy(destbuffer, resp.data(), resp.size());
  // 0:  copy back the content and free the elems buffer (ignored if not a copy).
  env->ReleasePrimitiveArrayCritical(result, destbuffer, 0);
  return result;
}

/*
 * Class:     com_ibm_geds_GEDSFile
 * Method:    setMetadataNative
 * Signature: (JLjava/lang/String;Z)V
 */
JNIEXPORT void JNICALL Java_com_ibm_geds_GEDSFile_setMetadataNative__JLjava_lang_String_2Z(
    JNIEnv *env, jobject, jlong nativePtr, jstring jmetadata, jboolean seal) {
  if (nativePtr == 0) {
    throwNullPointerException(env, "The pointer representation is NULL!");
    return;
  }
  absl::Status status;
  auto *file = reinterpret_cast<GEDSFile *>(nativePtr); // NOLINT
  if (jmetadata == nullptr) {
    status = file->setMetadata(std::nullopt, seal);
  } else {
    auto metadata = env->GetStringUTFChars(jmetadata, nullptr);
    status = file->setMetadata(metadata, strlen(metadata), seal);
    env->ReleaseStringUTFChars(jmetadata, metadata);
  }
  if (!status.ok()) {
    throwIOException(env, status.message());
  }
  return;
}

/*
 * Class:     com_ibm_geds_GEDSFile
 * Method:    setMetadataNative
 * Signature: (JLjava/nio/ByteBuffer;IIZ)V
 */
JNIEXPORT void JNICALL Java_com_ibm_geds_GEDSFile_setMetadataNative__JLjava_nio_ByteBuffer_2IIZ(
    JNIEnv *env, jobject, jlong nativePtr, jobject jBuffer, jint offset, jint length,
    jboolean seal) {
  if (nativePtr == 0) {
    throwNullPointerException(env, "The pointer representation is NULL!");
    return;
  }
  absl::Status status;
  auto *file = reinterpret_cast<GEDSFile *>(nativePtr); // NOLINT
  if (jBuffer == nullptr) {
    status = file->setMetadata(std::nullopt, seal);
  } else {
    auto buffer = reinterpret_cast<const uint8_t *>(env->GetDirectBufferAddress(jBuffer)); // NOLINT
#ifndef NDEBUG
    auto capacity = env->GetDirectBufferCapacity(jBuffer);
    if ((size_t)offset + (size_t)length > (size_t)capacity) {
      throwRuntimeException(env, "Offset + length are bigger than buffer capacity.");
      return;
    }
#endif
    status = file->setMetadata(&buffer[offset], length, seal);
  }
  if (!status.ok()) {
    throwIOException(env, status.message());
  }
  return;
}

/*
 * Class:     com_ibm_geds_GEDSFile
 * Method:    setMetadataNative
 * Signature: (J[BIIZ)V
 */
JNIEXPORT void JNICALL Java_com_ibm_geds_GEDSFile_setMetadataNative__J_3BIIZ(
    JNIEnv *env, jobject, jlong nativePtr, jbyteArray jBuffer, jint offset, jint length,
    jboolean seal) {
  if (nativePtr == 0) {
    throwNullPointerException(env, "The pointer representation is NULL!");
    return;
  }
  absl::Status status;
  auto *file = reinterpret_cast<GEDSFile *>(nativePtr); // NOLINT
  if (jBuffer == nullptr) {
    status = file->setMetadata(std::nullopt, seal);
  } else {
    auto buffer =
        reinterpret_cast<jbyte *>(env->GetPrimitiveArrayCritical(jBuffer, nullptr)); // NOLINT
    if (buffer == nullptr) {
      // Operation failed.
      throwRuntimeException(env, "Unable to obtain primitive array.");
      return;
    }
    status = file->setMetadata(&buffer[offset], length, seal); // NOLINT
    env->ReleasePrimitiveArrayCritical(jBuffer, buffer, JNI_ABORT);
  }
  if (!status.ok()) {
    throwIOException(env, status.message());
  }
  return;
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

// NOLINTNEXTLINE
JNIEXPORT jint JNICALL Java_com_ibm_geds_GEDSFile_readNative__JJLjava_nio_ByteBuffer_2II(
    JNIEnv *env, jobject, jlong nativePtr, jlong position, jobject jBuffer, jint offset,
    jint length) {
  static auto counter = geds::Statistics::createIOHistogram("Java GEDSFile: bytes read");
  static auto timer = geds::Statistics::createNanoSecondHistogram("Java GEDSFile: read");
  auto timerBegin = std::chrono::high_resolution_clock::now();

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
  *timer += std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now() - timerBegin)
                .count();
  return *readStatus; // NOLINT
}

JNIEXPORT void JNICALL Java_com_ibm_geds_GEDSFile_writeNative__JJLjava_nio_ByteBuffer_2II(
    JNIEnv *env, jobject, jlong nativePtr, jlong position, jobject jBuffer, jint offset,
    jint length) {
  static auto counter = geds::Statistics::createIOHistogram("Java GEDSFile: bytes written");
  static auto timer = geds::Statistics::createNanoSecondHistogram("Java GEDSFile: write");
  auto timerBegin = std::chrono::high_resolution_clock::now();

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
