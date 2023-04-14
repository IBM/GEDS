package db

import (
	"github.com/IBM/gedsmds/internal/logger"
	"github.com/IBM/gedsmds/protos"
	"github.com/golang/protobuf/proto"
	"github.com/syndtr/goleveldb/leveldb"
	"github.com/syndtr/goleveldb/leveldb/util"
)

// NewOperations Possible optimization for LevelDB: https://github.com/google/leveldb/blob/master/doc/index.md
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
	if err := o.dbObjectStoreConfig.Put([]byte(o.createObjectStoreConfigKey(storeConfig)),
		dbValue, nil); err != nil {
		logger.ErrorLogger.Println(err)
	}
}

func (o *Operations) DeleteObjectStoreConfig(storeConfig *protos.ObjectStoreConfig) {
	if err := o.dbObjectStoreConfig.Delete([]byte(o.createObjectStoreConfigKey(storeConfig)), nil); err != nil {
		logger.ErrorLogger.Println(err)
	}
}

func (o *Operations) GetObjectStoreConfig(storeConfigQuery *protos.ObjectStoreConfig) (
	*protos.ObjectStoreConfig, error) {
	objectStoreConfig := &protos.ObjectStoreConfig{}
	if valueBytes, err := o.dbObjectStoreConfig.Get([]byte(o.createObjectStoreConfigKey(storeConfigQuery)),
		nil); err != nil {
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
	iter := o.dbObjectStoreConfig.NewIterator(util.BytesPrefix([]byte(o.createObjectStoreConfigKeyPrefix())), nil)
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
	if err := o.dbBucket.Put([]byte(o.createBucketKey(bucket)), dbValue, nil); err != nil {
		logger.ErrorLogger.Println(err)
	}
}

func (o *Operations) DeleteBucket(bucket *protos.Bucket) {
	if err := o.dbBucket.Delete([]byte(o.createBucketKey(bucket)), nil); err != nil {
		logger.ErrorLogger.Println(err)
	}
	iter := o.dbObject.NewIterator(util.BytesPrefix([]byte(o.createObjectKeyPrefixWithBucket(&protos.Object{
		Id: &protos.ObjectID{
			Bucket: bucket.Bucket,
		},
	}))), nil)
	for iter.Next() {
		if err := o.dbObject.Delete(iter.Key(), nil); err != nil {
			logger.ErrorLogger.Println(err)
		}
	}
	iter.Release()
}

func (o *Operations) GetBucket(bucketQuery *protos.Bucket) (*protos.Bucket, error) {
	bucket := &protos.Bucket{}
	if valueBytes, err := o.dbBucket.Get([]byte(o.createBucketKey(bucketQuery)), nil); err != nil {
		return nil, err
	} else {
		if err = proto.Unmarshal(valueBytes, bucket); err != nil {
			return nil, err
		}
	}
	return bucket, nil
}

func (o *Operations) LookupBucket(bucketQuery *protos.Bucket) error {
	if _, err := o.dbBucket.Get([]byte(o.createBucketKey(bucketQuery)), nil); err != nil {
		return err
	} else {
		return nil
	}
}

func (o *Operations) GetAllBuckets() (
	[]*protos.Bucket, error) {
	var buckets []*protos.Bucket
	iter := o.dbBucket.NewIterator(util.BytesPrefix([]byte(o.createBucketKeyPrefix())), nil)
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
	if err := o.dbObject.Put([]byte(o.createObjectKey(object)), dbValue, nil); err != nil {
		logger.ErrorLogger.Println(err)
	}
}

func (o *Operations) DeleteObject(object *protos.Object) {
	if err := o.dbObject.Delete([]byte(o.createObjectKey(object)), nil); err != nil {
		logger.ErrorLogger.Println(err)
	}
}

func (o *Operations) GetObject(objectQuery *protos.Object) (*protos.Object, error) {
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

func (o *Operations) GetAllObjectsInBucket(objectQuery *protos.Object) (map[string]*protos.Object, error) {
	objects := map[string]*protos.Object{}
	iter := o.dbObject.NewIterator(util.BytesPrefix([]byte(o.createObjectKeyPrefixWithBucket(objectQuery))), nil)
	for iter.Next() {
		value := &protos.Object{}
		if err := proto.Unmarshal(iter.Value(), value); err != nil {
			return nil, err
		}
		objects[string(iter.Key())] = value
	}
	iter.Release()
	return objects, iter.Error()
}
