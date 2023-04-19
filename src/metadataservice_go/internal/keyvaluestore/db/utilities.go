package db

import (
	"github.com/IBM/gedsmds/protos"
)

const CommonDelimiter = "/"

func (o *Operations) createObjectKey(objectID *protos.ObjectID) string {
	return objectID.Bucket + CommonDelimiter + objectID.Key
}

func (o *Operations) createObjectKeyWithBucketPrefix(objectID *protos.ObjectID) string {
	return objectID.Bucket + CommonDelimiter
}
