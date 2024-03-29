/**
 * Copyright 2023- Pezhman Nasirifard. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

package db

import (
	"github.com/IBM/gedsmds/internal/logger"
	"github.com/IBM/gedsmds/protos"
	"github.com/golang/protobuf/proto"
	"github.com/syndtr/goleveldb/leveldb"
	"github.com/syndtr/goleveldb/leveldb/util"
)

func NewOperations() *Operations {
	operations := &Operations{}
	if db, err := leveldb.OpenFile(objectStoreConfigLocation, nil); err == nil {
		operations.dbObjectStoreConfig = db
	} else {
		logger.FatalLogger.Fatalln(err)
	}
	if db, err := leveldb.OpenFile(bucketLocation, nil); err == nil {
		operations.dbBucket = db
	} else {
		logger.FatalLogger.Fatalln(err)
	}
	if db, err := leveldb.OpenFile(objectLocation, nil); err == nil {
		operations.dbObject = db
	} else {
		logger.FatalLogger.Fatalln(err)
	}
	operations.ObjectStoreConfigChan = make(chan *OperationParams, channelBufferSize)
	operations.BucketChan = make(chan *OperationParams, channelBufferSize)
	operations.ObjectChan = make(chan *OperationParams, channelBufferSize)

	go operations.runDBOperationsListener()

	return operations
}

func (o *Operations) runDBOperationsListener() {
	for {
		select {
		case operationParams := <-o.ObjectStoreConfigChan:
			if operationParams.Type == PUT {
				o.PutObjectStoreConfig(operationParams.ObjectStoreConfig)
			} else if operationParams.Type == DELETE {
				o.DeleteObjectStoreConfig(operationParams.ObjectStoreConfig)
			}
		case operationParams := <-o.BucketChan:
			if operationParams.Type == PUT {
				o.PutBucket(operationParams.Bucket)
			} else if operationParams.Type == DELETE {
				o.DeleteBucket(operationParams.Bucket)
			}
		case operationParams := <-o.ObjectChan:
			if operationParams.Type == PUT {
				o.PutObject(operationParams.Object)
			} else if operationParams.Type == DELETE {
				o.DeleteObject(operationParams.Object)
			}
		}
	}
}

func (o *Operations) PutObjectStoreConfig(storeConfig *protos.ObjectStoreConfig) {
	dbValue, _ := proto.Marshal(storeConfig)
	if err := o.dbObjectStoreConfig.Put([]byte(storeConfig.Bucket),
		dbValue, nil); err != nil {
		logger.ErrorLogger.Println(err)
	}
}

func (o *Operations) DeleteObjectStoreConfig(storeConfig *protos.ObjectStoreConfig) {
	if err := o.dbObjectStoreConfig.Delete([]byte(storeConfig.Bucket), nil); err != nil {
		logger.ErrorLogger.Println(err)
	}
}

func (o *Operations) GetObjectStoreConfig(storeConfigQuery *protos.ObjectStoreConfig) (
	*protos.ObjectStoreConfig, error) {
	objectStoreConfig := &protos.ObjectStoreConfig{}
	if valueBytes, err := o.dbObjectStoreConfig.Get([]byte(storeConfigQuery.Bucket), nil); err != nil {
		return nil, err
	} else {
		if err = proto.Unmarshal(valueBytes, objectStoreConfig); err != nil {
			return nil, err
		}
	}
	return objectStoreConfig, nil
}

func (o *Operations) GetAllObjectStoreConfig() (
	[]*protos.ObjectStoreConfig, error) {
	var objectStoreConfigs []*protos.ObjectStoreConfig
	iter := o.dbObjectStoreConfig.NewIterator(nil, nil)
	for iter.Next() {
		value := &protos.ObjectStoreConfig{}
		if err := proto.Unmarshal(iter.Value(), value); err != nil {
			return nil, err
		}
		objectStoreConfigs = append(objectStoreConfigs, value)
	}
	iter.Release()
	return objectStoreConfigs, iter.Error()
}

func (o *Operations) PutBucket(bucket *protos.Bucket) {
	dbValue, _ := proto.Marshal(bucket)
	if err := o.dbBucket.Put([]byte(bucket.Bucket), dbValue, nil); err != nil {
		logger.ErrorLogger.Println(err)
	}
}

func (o *Operations) DeleteBucket(bucket *protos.Bucket) {
	if err := o.dbBucket.Delete([]byte(bucket.Bucket), nil); err != nil {
		logger.ErrorLogger.Println(err)
	}
	iter := o.dbObject.NewIterator(util.BytesPrefix([]byte(o.createObjectKeyWithBucketPrefix(&protos.ObjectID{
		Bucket: bucket.Bucket}))), nil)
	for iter.Next() {
		if err := o.dbObject.Delete(iter.Key(), nil); err != nil {
			logger.ErrorLogger.Println(err)
		}
	}
	iter.Release()
}

func (o *Operations) GetBucket(bucketQuery *protos.Bucket) (*protos.Bucket, error) {
	bucket := &protos.Bucket{}
	if valueBytes, err := o.dbBucket.Get([]byte(bucketQuery.Bucket), nil); err != nil {
		return nil, err
	} else {
		if err = proto.Unmarshal(valueBytes, bucket); err != nil {
			return nil, err
		}
	}
	return bucket, nil
}

func (o *Operations) LookupBucket(bucketQuery *protos.Bucket) error {
	if _, err := o.dbBucket.Get([]byte(bucketQuery.Bucket), nil); err != nil {
		return err
	} else {
		return nil
	}
}

func (o *Operations) GetAllBuckets() (
	[]*protos.Bucket, error) {
	var buckets []*protos.Bucket
	iter := o.dbBucket.NewIterator(nil, nil)
	for iter.Next() {
		value := &protos.Bucket{}
		if err := proto.Unmarshal(iter.Value(), value); err != nil {
			return nil, err
		}
		buckets = append(buckets, value)
	}
	iter.Release()
	return buckets, iter.Error()
}

func (o *Operations) PutObject(object *protos.Object) {
	dbValue, _ := proto.Marshal(object)
	if err := o.dbObject.Put([]byte(o.createObjectKey(object.Id)), dbValue, nil); err != nil {
		logger.ErrorLogger.Println(err)
	}
}

func (o *Operations) DeleteObject(object *protos.Object) {
	if err := o.dbObject.Delete([]byte(o.createObjectKey(object.Id)), nil); err != nil {
		logger.ErrorLogger.Println(err)
	}
}

func (o *Operations) GetObjectWithObjectID(objectQuery *protos.ObjectID) (*protos.Object, error) {
	object := &protos.Object{}
	if valueBytes, err := o.dbObject.Get([]byte(o.createObjectKey(objectQuery)), nil); err != nil {
		return nil, err
	} else {
		if err = proto.Unmarshal(valueBytes, object); err != nil {
			return nil, err
		}
	}
	return object, nil
}

func (o *Operations) GetObjectWithStringID(objectID string) (*protos.Object, error) {
	object := &protos.Object{}
	if valueBytes, err := o.dbObject.Get([]byte(objectID), nil); err != nil {
		return nil, err
	} else {
		if err = proto.Unmarshal(valueBytes, object); err != nil {
			return nil, err
		}
	}
	return object, nil
}

func (o *Operations) GetAllObjectsPrefix(objectQuery *protos.ObjectID) ([]*protos.Object, error) {
	objects := []*protos.Object{}
	iter := o.dbObject.NewIterator(util.BytesPrefix([]byte(o.createObjectKey(objectQuery))), nil)
	for iter.Next() {
		value := &protos.Object{}
		if err := proto.Unmarshal(iter.Value(), value); err != nil {
			return nil, err
		}
		objects = append(objects, value)
	}
	iter.Release()
	return objects, iter.Error()
}

func (o *Operations) GetAllObjectsIDsPrefix(objectQuery *protos.ObjectID) ([]string, error) {
	objectsIDs := []string{}
	iter := o.dbObject.NewIterator(util.BytesPrefix([]byte(o.createObjectKey(objectQuery))), nil)
	for iter.Next() {
		objectsIDs = append(objectsIDs, string(iter.Key()))
	}
	iter.Release()
	return objectsIDs, iter.Error()
}
