package db

import (
	"github.com/IBM/gedsmds/protos"
)

const objectStoreConfigPrefix = "c-"
const bucketPrefix = "b-"
const objectPrefix = "o-"

func (o *Operations) createObjectStoreConfigKey(objectStoreConfig *protos.ObjectStoreConfig) string {
	return objectStoreConfigPrefix + "-" + objectStoreConfig.Bucket
}

func (o *Operations) createObjectStoreConfigKeyPrefix() string {
	return objectStoreConfigPrefix + "-"
}

func (o *Operations) createBucketKey(bucket *protos.Bucket) string {
	return bucketPrefix + "-" + bucket.Bucket
}

func (o *Operations) createBucketKeyPrefix() string {
	return bucketPrefix + "-"
}

func (o *Operations) createObjectKey(object *protos.Object) string {
	return objectPrefix + "-" + object.Id.Bucket + "-" + object.Id.Key
}

func (o *Operations) createObjectKeyPrefixWithBucket(object *protos.Object) string {
	return objectPrefix + "-" + object.Id.Bucket + "-"
}

func (o *Operations) createObjectKeyPrefixWithoutBucket() string {
	return objectPrefix + "-"
}
