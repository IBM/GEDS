/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "com_ibm_geds_GEDS.h"

#include <climits>
#include <cstddef>
#include <optional>
#include <string>

#include "GEDS.h"
#include "GEDSConfig.h"
#include "GEDSConfigContainer.h"
#include "JavaError.h"
#include "Logging.h"
#include "Ports.h"
#include "Statistics.h"

// NOLINTBEGIN(modernize-use-trailing-return-type)
struct GEDSContainer {
  std::shared_ptr<GEDS> element;
};

/*
 * Class:     com_ibm_geds_GEDS
 * Method:    initGEDS
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_com_ibm_geds_GEDS_initGEDS(JNIEnv *env, jclass,
                                                        jlong nativePtrConfig) {
  if (nativePtrConfig == 0) {
    throwNullPointerException(env, "Invalid nativePtr.");
    return 0;
  }
  auto container = reinterpret_cast<GEDSConfigContainer *>(nativePtrConfig); // NOLINT

  auto geds = GEDS::factory(*(container->element));

  auto status = geds->start();
  if (!status.ok()) {
    return throwRuntimeException(env, std::string{status.message()});
  }
  return reinterpret_cast<jlong>(new GEDSContainer{geds}); // NOLINT
}

JNIEXPORT void JNICALL Java_com_ibm_geds_GEDS_nativeStopGEDS(JNIEnv *env, jclass, jlong nativePtr) {
  if (nativePtr == 0) {
    throwNullPointerException(env, "Invalid nativePtr.");
    return;
  }
  LOG_INFO("Java Interop called: stop()");
  auto container = reinterpret_cast<GEDSContainer *>(nativePtr); // NOLINT
  (void)container->element->stop();
}

JNIEXPORT void JNICALL Java_com_ibm_geds_GEDS_printStatistics(JNIEnv *, jclass) {
  geds::Statistics::print();
}

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
JNIEXPORT jint JNICALL Java_com_ibm_geds_GEDS_getDefaultGEDSPort(JNIEnv *, jclass) {
  return defaultGEDSPort;
}

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
JNIEXPORT jint JNICALL Java_com_ibm_geds_GEDS_getDefaultMetdataServerPort(JNIEnv *, jclass) {
  return defaultMetdataServerPort;
}

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
JNIEXPORT jlong JNICALL Java_com_ibm_geds_GEDS_nativeCreate(JNIEnv *env, jclass, jlong nativePtr,
                                                            jstring jBucket, jstring jKey) {
  static auto counter = geds::Statistics::createCounter("Java GEDS: create");

  if (nativePtr == 0) {
    return throwNullPointerException(env, "Invalid nativePtr.");
  }
  auto container = reinterpret_cast<GEDSContainer *>(nativePtr); // NOLINT

  auto bucket = env->GetStringUTFChars(jBucket, nullptr);
  auto key = env->GetStringUTFChars(jKey, nullptr);
  auto createStatus = container->element->create(std::string(bucket), std::string(key));
  env->ReleaseStringUTFChars(jBucket, bucket);
  env->ReleaseStringUTFChars(jKey, key);
  *counter += 1;
  if (createStatus.ok()) {
    static_assert(sizeof(GEDSFile *) == sizeof(jlong)); // NOLINT(bugprone-sizeof-expression)
    return reinterpret_cast<jlong>(new GEDSFile(createStatus.value())); // NOLINT
  }
  return throwIOException(env, createStatus.status().message());
}

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
JNIEXPORT jboolean JNICALL Java_com_ibm_geds_GEDS_nativeRename(JNIEnv *env, jclass, jlong nativePtr,
                                                               jstring jSrcBucket, jstring jSrcKey,
                                                               jstring jDestBucket,
                                                               jstring jDestKey) {
  static auto counter = geds::Statistics::createCounter("Java GEDS: rename");

  if (nativePtr == 0) {
    return throwNullPointerException(env, "Invalid nativePtr.");
  }
  auto container = reinterpret_cast<GEDSContainer *>(nativePtr); // NOLINT

  auto srcBucket = env->GetStringUTFChars(jSrcBucket, nullptr);
  auto srcKey = env->GetStringUTFChars(jSrcKey, nullptr);
  auto destBucket = env->GetStringUTFChars(jDestBucket, nullptr);
  auto destKey = env->GetStringUTFChars(jDestKey, nullptr);
  auto status = container->element->rename(srcBucket, srcKey, destBucket, destKey);
  env->ReleaseStringUTFChars(jSrcBucket, srcBucket);
  env->ReleaseStringUTFChars(jSrcKey, srcKey);
  env->ReleaseStringUTFChars(jDestBucket, destBucket);
  env->ReleaseStringUTFChars(jDestKey, destKey);
  *counter += 1;
  if (status.ok()) {
    return true;
  }
  if (status.code() == absl::StatusCode::kNotFound) {
    throwFileNotFoundException(env, status.message());
  } else {
    throwIOException(env, status.message());
  }
  return false;
}

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
JNIEXPORT jboolean JNICALL
Java_com_ibm_geds_GEDS_nativeRenamePrefix(JNIEnv *env, jclass, jlong nativePtr, jstring jSrcBucket,
                                          jstring jSrcKey, jstring jDestBucket, jstring jDestKey) {
  static auto counter = geds::Statistics::createCounter("Java GEDS: rename prefix");

  if (nativePtr == 0) {
    return throwNullPointerException(env, "Invalid nativePtr.");
  }
  auto container = reinterpret_cast<GEDSContainer *>(nativePtr); // NOLINT

  auto srcBucket = env->GetStringUTFChars(jSrcBucket, nullptr);
  auto srcKey = env->GetStringUTFChars(jSrcKey, nullptr);
  auto destBucket = env->GetStringUTFChars(jDestBucket, nullptr);
  auto destKey = env->GetStringUTFChars(jDestKey, nullptr);
  auto status = container->element->renamePrefix(srcBucket, srcKey, destBucket, destKey);
  env->ReleaseStringUTFChars(jSrcBucket, srcBucket);
  env->ReleaseStringUTFChars(jSrcKey, srcKey);
  env->ReleaseStringUTFChars(jDestBucket, destBucket);
  env->ReleaseStringUTFChars(jDestKey, destKey);
  *counter += 1;
  if (status.ok()) {
    return true;
  }
  if (status.code() == absl::StatusCode::kNotFound) {
    throwFileNotFoundException(env, status.message());
  } else {
    throwIOException(env, status.message());
  }
  return false;
}

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
JNIEXPORT jboolean JNICALL Java_com_ibm_geds_GEDS_nativeCopy(JNIEnv *env, jclass, jlong nativePtr,
                                                             jstring jSrcBucket, jstring jSrcKey,
                                                             jstring jDestBucket,
                                                             jstring jDestKey) {
  if (nativePtr == 0) {
    return throwNullPointerException(env, "Invalid nativePtr.");
  }
  auto container = reinterpret_cast<GEDSContainer *>(nativePtr); // NOLINT
  auto srcBucket = env->GetStringUTFChars(jSrcBucket, nullptr);
  auto srcKey = env->GetStringUTFChars(jSrcKey, nullptr);
  auto destBucket = env->GetStringUTFChars(jDestBucket, nullptr);
  auto destKey = env->GetStringUTFChars(jDestKey, nullptr);

  auto status = container->element->copy(srcBucket, srcKey, destBucket, destKey);
  env->ReleaseStringUTFChars(jSrcBucket, srcBucket);
  env->ReleaseStringUTFChars(jSrcKey, srcKey);
  env->ReleaseStringUTFChars(jDestBucket, destBucket);
  env->ReleaseStringUTFChars(jDestKey, destKey);
  if (status.ok()) {
    return true;
  }
  if (status.code() == absl::StatusCode::kNotFound) {
    throwFileNotFoundException(env, status.message());
  } else {
    throwIOException(env, status.message());
  }
  return false;
}

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
JNIEXPORT jboolean JNICALL
Java_com_ibm_geds_GEDS_nativeCopyPrefix(JNIEnv *env, jclass, jlong nativePtr, jstring jSrcBucket,
                                        jstring jSrcKey, jstring jDestBucket, jstring jDestKey) {
  if (nativePtr == 0) {
    return throwNullPointerException(env, "Invalid nativePtr.");
  }
  auto container = reinterpret_cast<GEDSContainer *>(nativePtr); // NOLINT
  auto srcBucket = env->GetStringUTFChars(jSrcBucket, nullptr);
  auto srcKey = env->GetStringUTFChars(jSrcKey, nullptr);
  auto destBucket = env->GetStringUTFChars(jDestBucket, nullptr);
  auto destKey = env->GetStringUTFChars(jDestKey, nullptr);

  auto status = container->element->copyPrefix(srcBucket, srcKey, destBucket, destKey);
  env->ReleaseStringUTFChars(jSrcBucket, srcBucket);
  env->ReleaseStringUTFChars(jSrcKey, srcKey);
  env->ReleaseStringUTFChars(jDestBucket, destBucket);
  env->ReleaseStringUTFChars(jDestKey, destKey);
  if (status.ok()) {
    return true;
  }
  if (status.code() == absl::StatusCode::kNotFound) {
    throwFileNotFoundException(env, status.message());
  } else {
    throwIOException(env, status.message());
  }
  return false;
}

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
JNIEXPORT jlong JNICALL Java_com_ibm_geds_GEDS_nativeOpen(JNIEnv *env, jclass, jlong nativePtr,
                                                          jstring jBucket, jstring jKey) {
  static auto counter = geds::Statistics::createCounter("Java GEDS: open");

  if (nativePtr == 0) {
    return throwNullPointerException(env, "Invalid nativePtr.");
  }
  auto container = reinterpret_cast<GEDSContainer *>(nativePtr); // NOLINT

  auto bucket = env->GetStringUTFChars(jBucket, nullptr);
  auto key = env->GetStringUTFChars(jKey, nullptr);
  auto status = container->element->open(std::string(bucket), std::string(key));
  env->ReleaseStringUTFChars(jBucket, bucket);
  env->ReleaseStringUTFChars(jKey, key);
  *counter += 1;
  if (status.ok()) {
    static_assert(sizeof(GEDSFile *) == sizeof(jlong)); // NOLINT(bugprone-sizeof-expression)
    return reinterpret_cast<jlong>(new GEDSFile(status.value())); // NOLINT
  }
  if (status.status().code() == absl::StatusCode::kNotFound) {
    throwFileNotFoundException(env, status.status().message());
  } else {
    throwIOException(env, status.status().message());
  }
  return 0;
}

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
JNIEXPORT jboolean JNICALL Java_com_ibm_geds_GEDS_nativeCreateBucket(JNIEnv *env, jclass,
                                                                     jlong nativePtr,
                                                                     jstring jBucket) {
  static auto counter = geds::Statistics::createCounter("Java GEDS: create bucket");

  if (nativePtr == 0) {
    return throwNullPointerException(env, "Invalid nativePtr.");
  }
  auto container = reinterpret_cast<GEDSContainer *>(nativePtr); // NOLINT

  auto bucket = env->GetStringUTFChars(jBucket, nullptr);
  auto createStatus = container->element->createBucket(bucket);
  env->ReleaseStringUTFChars(jBucket, bucket);
  *counter += 1;
  if (createStatus.ok() || createStatus.code() == absl::StatusCode::kAlreadyExists) {
    return true;
  }
  throwIOException(env, createStatus.message());
  return false;
}

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
JNIEXPORT jboolean JNICALL Java_com_ibm_geds_GEDS_nativeMkdirs(JNIEnv *env, jclass, jlong nativePtr,
                                                               jstring jBucket, jstring jKey) {
  if (nativePtr == 0) {
    throwNullPointerException(env, "Invalid nativePtr.");
    return false;
  }
  static auto counter = geds::Statistics::createCounter("Java GEDS: mkdirs");

  auto container = reinterpret_cast<GEDSContainer *>(nativePtr); // NOLINT

  auto bucket = env->GetStringUTFChars(jBucket, nullptr);
  auto key = env->GetStringUTFChars(jKey, nullptr);
  auto status = container->element->mkdirs(bucket, key);
  env->ReleaseStringUTFChars(jBucket, bucket);
  env->ReleaseStringUTFChars(jKey, key);
  *counter += 1;
  if (status.ok()) {
    return true;
  }
  throwIOException(env, status.message());
  return false;
}

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
JNIEXPORT jboolean JNICALL Java_com_ibm_geds_GEDS_nativeDelete(JNIEnv *env, jclass, jlong nativePtr,
                                                               jstring jBucket, jstring jKey) {
  static auto counter = geds::Statistics::createCounter("Java GEDS: delete");

  if (nativePtr == 0) {
    return throwNullPointerException(env, "Invalid nativePtr.");
  }
  auto container = reinterpret_cast<GEDSContainer *>(nativePtr); // NOLINT

  auto bucket = env->GetStringUTFChars(jBucket, nullptr);
  auto key = env->GetStringUTFChars(jKey, nullptr);
  auto status = container->element->deleteObject(bucket, key);
  env->ReleaseStringUTFChars(jBucket, bucket);
  env->ReleaseStringUTFChars(jKey, key);
  *counter += 1;
  if (status.ok()) {
    return true;
  }
  if (status.code() == absl::StatusCode::kNotFound) {
    throwFileNotFoundException(env, status.message());
  } else {
    throwIOException(env, status.message());
  }
  return false;
}

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
JNIEXPORT jboolean JNICALL Java_com_ibm_geds_GEDS_nativeDeletePrefix(JNIEnv *env, jclass,
                                                                     jlong nativePtr,
                                                                     jstring jBucket,
                                                                     jstring jKey) {
  static auto counter = geds::Statistics::createCounter("Java GEDS: delete prefix");

  if (nativePtr == 0) {
    return throwNullPointerException(env, "Invalid nativePtr.");
  }
  auto container = reinterpret_cast<GEDSContainer *>(nativePtr); // NOLINT

  auto bucket = env->GetStringUTFChars(jBucket, nullptr);
  auto key = env->GetStringUTFChars(jKey, nullptr);
  auto status = container->element->deleteObjectPrefix(bucket, key);
  env->ReleaseStringUTFChars(jBucket, bucket);
  env->ReleaseStringUTFChars(jKey, key);
  *counter += 1;
  if (status.ok()) {
    return true;
  }
  throwIOException(env, status.message());
  return false;
}

// NOLINTNEXTLINE
JNIEXPORT jobjectArray JNICALL Java_com_ibm_geds_GEDS_nativeList(JNIEnv *env, jclass,
                                                                 jlong nativePtr, jstring jBucket,
                                                                 jstring jKey, jchar delimiter,
                                                                 jboolean useCache) {
  static auto counter = geds::Statistics::createCounter("Java GEDS: list");

  if (nativePtr == 0) {
    throwNullPointerException(env, "Invalid nativePtr.");
    return nullptr;
  }
  auto container = reinterpret_cast<GEDSContainer *>(nativePtr); // NOLINT

  auto bucket = env->GetStringUTFChars(jBucket, nullptr);
  auto key = env->GetStringUTFChars(jKey, nullptr);
  auto listStatus = container->element->listFromCache(bucket, key, delimiter, useCache);
  env->ReleaseStringUTFChars(jBucket, bucket);
  env->ReleaseStringUTFChars(jKey, key);
  *counter += 1;
  if (listStatus.ok()) {
    auto jStatusClass = env->FindClass("com/ibm/geds/GEDSFileStatus");
    // https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/types.html
    auto jStatusClassInit = env->GetMethodID(jStatusClass, "<init>", "(Ljava/lang/String;JZ)V");
    const auto &resultVec = listStatus.value();
    if (resultVec.size() > INT_MAX) {
      LOG_ERROR("The result has size ", resultVec.size(),
                " which is bigger than INT_MAX: ", INT_MAX);
    }
    if (resultVec.empty()) {
      LOG_DEBUG("Empty result");
    }
    auto result = env->NewObjectArray(resultVec.size(), jStatusClass, nullptr);
    for (size_t i = 0; i < resultVec.size(); i++) {
      auto jKey = env->NewStringUTF(resultVec[i].key.c_str());
      auto jObj = env->NewObject(jStatusClass, jStatusClassInit, jKey, (jlong)(resultVec[i].size),
                                 resultVec[i].isDirectory);
      env->SetObjectArrayElement(result, i, jObj);
    }
    return result;
  }
  throwIOException(env, listStatus.status().message());
  return nullptr;
}

JNIEXPORT jobject JNICALL Java_com_ibm_geds_GEDS_nativeStatus(JNIEnv *env, jclass, jlong nativePtr,
                                                              jstring jBucket, jstring jKey,
                                                              jchar delimiter) {
  static auto counter = geds::Statistics::createCounter("Java GEDS: status");

  if (nativePtr == 0) {
    throwNullPointerException(env, "Invalid nativePtr.");
    return nullptr;
  }
  auto container = reinterpret_cast<GEDSContainer *>(nativePtr); // NOLINT

  auto bucket = env->GetStringUTFChars(jBucket, nullptr);
  auto key = env->GetStringUTFChars(jKey, nullptr);
  auto status = container->element->status(bucket, key, delimiter);
  env->ReleaseStringUTFChars(jBucket, bucket);
  env->ReleaseStringUTFChars(jKey, key);
  *counter += 1;
  if (status.ok()) {
    auto jStatusClass = env->FindClass("com/ibm/geds/GEDSFileStatus");
    // https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/types.html
    auto jStatusClassInit = env->GetMethodID(jStatusClass, "<init>", "(Ljava/lang/String;JZ)V");
    auto jKey = env->NewStringUTF(status->key.c_str());

    auto result = env->NewObject(jStatusClass, jStatusClassInit, jKey, (jlong)status->size,
                                 status->isDirectory);
    env->DeleteLocalRef(jStatusClass);
    env->DeleteLocalRef(jKey);
    return result;
  }
  if (status.status().code() == absl::StatusCode::kNotFound) {
    throwFileNotFoundException(env, status.status().message());
  } else {
    throwIOException(env, status.status().message());
  }
  return nullptr;
}

JNIEXPORT void JNICALL Java_com_ibm_geds_GEDS_nativeRegisterObjectStoreConfig(
    JNIEnv *env, jobject, jlong nativePtr, jstring jBucket, jstring jEndpointUrl,
    jstring jAccessKey, jstring jSecretKey) {
  if (nativePtr == 0) {
    throwNullPointerException(env, "Invalid nativePtr.");
    return;
  }
  auto container = reinterpret_cast<GEDSContainer *>(nativePtr); // NOLINT
  auto bucket = env->GetStringUTFChars(jBucket, nullptr);
  auto endpointUrl = env->GetStringUTFChars(jEndpointUrl, nullptr);
  auto accessKey = env->GetStringUTFChars(jAccessKey, nullptr);
  auto secretKey = env->GetStringUTFChars(jSecretKey, nullptr);
  auto status =
      container->element->registerObjectStoreConfig(bucket, endpointUrl, accessKey, secretKey);
  env->ReleaseStringUTFChars(jBucket, bucket);
  env->ReleaseStringUTFChars(jEndpointUrl, endpointUrl);
  env->ReleaseStringUTFChars(jAccessKey, accessKey);
  env->ReleaseStringUTFChars(jSecretKey, secretKey);
  if (!status.ok()) {
    throwRuntimeException(env, status.message());
  }
}

JNIEXPORT void JNICALL Java_com_ibm_geds_GEDS_nativeSyncObjectStoreConfigs(JNIEnv *env, jobject,
                                                                           jlong nativePtr) {
  if (nativePtr == 0) {
    throwNullPointerException(env, "Invalid nativePtr.");
    return;
  }
  auto container = reinterpret_cast<GEDSContainer *>(nativePtr); // NOLINT
  auto status = container->element->syncObjectStoreConfigs();
  if (!status.ok()) {
    throwRuntimeException(env, status.message());
  }
}

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
JNIEXPORT jboolean JNICALL Java_com_ibm_geds_GEDS_nativeSubscribeStreamWithThread(JNIEnv *env,
                                                                                  jclass,
                                                                                  jlong nativePtr) {
  if (nativePtr == 0) {
    return throwNullPointerException(env, "Invalid nativePtr.");
  }
  auto container = reinterpret_cast<GEDSContainer *>(nativePtr); // NOLINT
  auto status = container->element->subscribeStreamWithThread(geds::SubscriptionEvent{});
  if (!status.ok()) {
    throwIOException(env, status.message());
  }
  return JNI_TRUE;
}

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
JNIEXPORT jboolean JNICALL Java_com_ibm_geds_GEDS_nativeStopSubscribeStreamWithThread(JNIEnv *env,
                                                                                  jclass,
                                                                                  jlong nativePtr) {
  if (nativePtr == 0) {
    return throwNullPointerException(env, "Invalid nativePtr.");
  }
  auto container = reinterpret_cast<GEDSContainer *>(nativePtr); // NOLINT
  auto status = container->element->stopSubscribeStreamWithThread();
  if (!status.ok()) {
    throwIOException(env, status.message());
  }
  return JNI_TRUE;
}

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
JNIEXPORT jboolean JNICALL Java_com_ibm_geds_GEDS_nativeSubscribe(JNIEnv *env, jclass,
                                                                  jlong nativePtr, jstring jBucket,
                                                                  jstring jKey,
                                                                  jint jSubscriptionType) {
  if (nativePtr == 0) {
    return throwNullPointerException(env, "Invalid nativePtr.");
  }
  auto container = reinterpret_cast<GEDSContainer *>(nativePtr); // NOLINT

  auto bucket = env->GetStringUTFChars(jBucket, nullptr);
  auto key = env->GetStringUTFChars(jKey, nullptr);
  auto status = container->element->subscribe(geds::SubscriptionEvent{
      "", bucket, key, static_cast<geds::rpc::SubscriptionType>(jSubscriptionType)});
  env->ReleaseStringUTFChars(jBucket, bucket);
  env->ReleaseStringUTFChars(jKey, key);

  if (!status.ok()) {
    throwIOException(env, status.message());
  }
  return JNI_TRUE;
}

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
JNIEXPORT jboolean JNICALL Java_com_ibm_geds_GEDS_nativeUnsubscribe(JNIEnv *env, jclass,
                                                                    jlong nativePtr,
                                                                    jstring jBucket, jstring jKey,
                                                                    jint jSubscriptionType) {
  if (nativePtr == 0) {
    return throwNullPointerException(env, "Invalid nativePtr.");
  }
  auto container = reinterpret_cast<GEDSContainer *>(nativePtr); // NOLINT

  auto bucket = env->GetStringUTFChars(jBucket, nullptr);
  auto key = env->GetStringUTFChars(jKey, nullptr);
  auto status = container->element->unsubscribe(geds::SubscriptionEvent{
      "", bucket, key, static_cast<geds::rpc::SubscriptionType>(jSubscriptionType)});

  env->ReleaseStringUTFChars(jBucket, bucket);
  env->ReleaseStringUTFChars(jKey, key);

  if (!status.ok()) {
    throwIOException(env, status.message());
  }
  return JNI_TRUE;
}

/*
 * Class:     com_ibm_geds_GEDS
 * Method:    nativeRelocate
 * Signature: (JZ)V
 */
JNIEXPORT void JNICALL Java_com_ibm_geds_GEDS_nativeRelocate(JNIEnv *env, jobject, jlong nativePtr,
                                                             jboolean force) {
  if (nativePtr == 0) {
    throwNullPointerException(env, "Invalid nativePtr.");
    return;
  }
  auto container = reinterpret_cast<GEDSContainer *>(nativePtr); // NOLINT
  container->element->relocate(force);
}

// NOLINTEND(modernize-use-trailing-return-type)
