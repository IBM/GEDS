/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "com_ibm_geds_GEDSConfig.h"

#include <cstdint>
#include <memory>
#include <string>

#include "GEDSConfig.h"
#include "GEDSConfigContainer.h"
#include "JavaError.h"
#include "Logging.h"

/*
 * Class:     com_ibm_geds_GEDSConfig
 * Method:    initGEDSConfig
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_com_ibm_geds_GEDSConfig_initGEDSConfig(JNIEnv *env, jclass,
                                                                    jstring addressJava) {
  auto address = env->GetStringUTFChars(addressJava, nullptr);
  auto addressStr = std::string{address};

  auto config = std::make_shared<GEDSConfig>(addressStr);

  env->ReleaseStringUTFChars(addressJava, address);
  return reinterpret_cast<jlong>(new GEDSConfigContainer{config}); // NOLINT
}

/*
 * Class:     com_ibm_geds_GEDSConfig
 * Method:    nativeSetString
 * Signature: (JLjava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_ibm_geds_GEDSConfig_nativeSetString(JNIEnv *env, jclass,
                                                                    jlong nativePtr, jstring jkey,
                                                                    jstring jvalue) {
  if (nativePtr == 0) {
    throwNullPointerException(env, "Invalid nativePtr.");
    return;
  }
  auto key = env->GetStringUTFChars(jkey, nullptr);
  auto value = env->GetStringUTFChars(jvalue, nullptr);

  auto container = reinterpret_cast<GEDSConfigContainer *>(nativePtr); // NOLINT
  auto status = container->element->set(key, value);
  if (!status.ok()) {
    LOG_ERROR("Unable to set ", key, status.message());
  }
  env->ReleaseStringUTFChars(jkey, key);
  env->ReleaseStringUTFChars(jvalue, value);
}

/*
 * Class:     com_ibm_geds_GEDSConfig
 * Method:    nativeSetInt
 * Signature: (JLjava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_com_ibm_geds_GEDSConfig_nativeSetInt(JNIEnv *env, jclass,
                                                                 jlong nativePtr, jstring jKey,
                                                                 jint value) {
  if (nativePtr == 0) {
    throwNullPointerException(env, "Invalid nativePtr.");
    return;
  }
  auto key = env->GetStringUTFChars(jKey, nullptr);
  auto container = reinterpret_cast<GEDSConfigContainer *>(nativePtr); // NOLINT
  auto status = container->element->set(key, (int64_t)value);
  if (!status.ok()) {
    LOG_ERROR("Unable to set ", key, status.message());
  }
  env->ReleaseStringUTFChars(jKey, key);
}


/*
 * Class:     com_ibm_geds_GEDSConfig
 * Method:    nativeSetLong
 * Signature: (JLjava/lang/String;J)V
 */
JNIEXPORT void JNICALL Java_com_ibm_geds_GEDSConfig_nativeSetLong(JNIEnv *env, jclass,
                                                                  jlong nativePtr, jstring jKey,
                                                                  jlong value) {
  if (nativePtr == 0) {
    throwNullPointerException(env, "Invalid nativePtr.");
    return;
  }
  auto key = env->GetStringUTFChars(jKey, nullptr);
  auto container = reinterpret_cast<GEDSConfigContainer *>(nativePtr); // NOLINT
  auto status = container->element->set(key, (int64_t)value);
  if (!status.ok()) {
    LOG_ERROR("Unable to set ", key, status.message());
  }
  env->ReleaseStringUTFChars(jKey, key);
}

/*
 * Class:     com_ibm_geds_GEDSConfig
 * Method:    nativeGetString
 * Signature: (JLjava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ibm_geds_GEDSConfig_nativeGetString(JNIEnv *env, jclass,
                                                                       jlong nativePtr,
                                                                       jstring jKey) {
  if (nativePtr == 0) {
    throwNullPointerException(env, "Invalid nativePtr.");
    return nullptr;
  }
  auto key = env->GetStringUTFChars(jKey, nullptr);
  auto container = reinterpret_cast<GEDSConfigContainer *>(nativePtr); // NOLINT
  auto status = container->element->getString(key);
  if (!status.ok()) {
    LOG_ERROR("Unable to read ", key, " as string: ", status.status().message());
    env->ReleaseStringUTFChars(jKey, key);
    throwIOException(env, status.status().message());
    return nullptr;
  }
  env->ReleaseStringUTFChars(jKey, key);
  return env->NewStringUTF(status->c_str());
}

/*
 * Class:     com_ibm_geds_GEDSConfig
 * Method:    nativeGetInt
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_geds_GEDSConfig_nativeGetInt(JNIEnv *env, jclass,
                                                                 jlong nativePtr, jstring jKey) {
  if (nativePtr == 0) {
    throwNullPointerException(env, "Invalid nativePtr.");
    return 0;
  }
  auto key = env->GetStringUTFChars(jKey, nullptr);
  auto container = reinterpret_cast<GEDSConfigContainer *>(nativePtr); // NOLINT
  auto status = container->element->getSignedInt(key);

  if (!status.ok()) {
    LOG_ERROR("Unable to read ", key, " as int: ", status.status().message());
    env->ReleaseStringUTFChars(jKey, key);
    throwIOException(env, status.status().message());
    return 0;
  }
  env->ReleaseStringUTFChars(jKey, key);
  return (jint)*status;
}

/*
 * Class:     com_ibm_geds_GEDSConfig
 * Method:    nativeGetLong
 * Signature: (JLjava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_com_ibm_geds_GEDSConfig_nativeGetLong(JNIEnv *env, jclass,
                                                                   jlong nativePtr, jstring jKey) {
  if (nativePtr == 0) {
    throwNullPointerException(env, "Invalid nativePtr.");
    return 0;
  }
  auto key = env->GetStringUTFChars(jKey, nullptr);
  auto container = reinterpret_cast<GEDSConfigContainer *>(nativePtr); // NOLINT
  auto status = container->element->getSignedInt(key);
  if (!status.ok()) {
    LOG_ERROR("Unable to read ", key, " as long: ", status.status().message());
    env->ReleaseStringUTFChars(jKey, key);
    throwIOException(env, status.status().message());
    return 0;
  }
  env->ReleaseStringUTFChars(jKey, key);
  return *status;
}
