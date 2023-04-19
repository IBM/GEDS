package keyvaluestore

import (
	"errors"
	"github.com/IBM/gedsmds/internal/config"
	"github.com/IBM/gedsmds/internal/keyvaluestore/db"
	"github.com/IBM/gedsmds/internal/logger"
	"github.com/IBM/gedsmds/protos"
	"golang.org/x/exp/maps"
	"sync"
)

func InitKeyValueStoreService() *Service {
	kvStore := &Service{
		dbConnection: db.NewOperations(),

		objectStoreConfigsLock: &sync.RWMutex{},
		objectStoreConfigs:     map[string]*protos.ObjectStoreConfig{},

		bucketsLock: &sync.RWMutex{},
		buckets:     map[string]*Bucket{},
	}
	return kvStore
}

func (kv *Service) NewBucketIfNotExist(objectID *protos.ObjectID) (*Bucket, bool) {
	kv.bucketsLock.Lock()
	defer kv.bucketsLock.Unlock()
	if bucket, ok := kv.buckets[objectID.Bucket]; !ok {
		kv.buckets[objectID.Bucket] = &Bucket{
			bucket:            &protos.Bucket{Bucket: objectID.Bucket},
			objectsLock:       &sync.RWMutex{},
			nestedDirectories: makeNewPrefixTree("root"),
		}
		return kv.buckets[objectID.Bucket], false
	} else {
		return bucket, true
	}
}

func (kv *Service) RegisterObjectStore(objectStore *protos.ObjectStoreConfig) error {
	if config.Config.PersistentStorageEnabled {
		kv.dbConnection.ObjectStoreConfigChan <- &db.OperationParams{
			ObjectStoreConfig: objectStore,
			Type:              db.PUT,
		}
	} else {
		kv.objectStoreConfigsLock.Lock()
		defer kv.objectStoreConfigsLock.Unlock()
		if _, ok := kv.objectStoreConfigs[objectStore.Bucket]; ok {
			return errors.New("config already exists")
		}
		kv.objectStoreConfigs[objectStore.Bucket] = objectStore
	}
	return nil
}

func (kv *Service) ListObjectStores() (*protos.AvailableObjectStoreConfigs, error) {
	mappings := &protos.AvailableObjectStoreConfigs{Mappings: []*protos.ObjectStoreConfig{}}
	if config.Config.PersistentStorageEnabled {
		if allObjectStoreConfig, err := kv.dbConnection.GetAllObjectStoreConfig(); err != nil {
			logger.ErrorLogger.Println(err)
		} else {
			mappings.Mappings = append(mappings.Mappings, allObjectStoreConfig...)
		}
	} else {
		kv.objectStoreConfigsLock.RLock()
		defer kv.objectStoreConfigsLock.RUnlock()
		for _, objectStoreConfig := range kv.objectStoreConfigs {
			mappings.Mappings = append(mappings.Mappings, objectStoreConfig)
		}
	}
	return mappings, nil
}

func (kv *Service) CreateBucket(bucket *protos.Bucket) error {
	if config.Config.PersistentStorageEnabled {
		kv.dbConnection.BucketChan <- &db.OperationParams{
			Bucket: bucket,
			Type:   db.PUT,
		}
	} else {
		_, existed := kv.NewBucketIfNotExist(&protos.ObjectID{Bucket: bucket.Bucket})
		if existed {
			return errors.New("bucket already exists")
		}
	}
	return nil
}

func (kv *Service) DeleteBucket(bucket *protos.Bucket) error {
	if config.Config.PersistentStorageEnabled {
		kv.dbConnection.BucketChan <- &db.OperationParams{
			Bucket: bucket,
			Type:   db.DELETE,
		}
	} else {
		kv.bucketsLock.Lock()
		defer kv.bucketsLock.Unlock()
		if _, ok := kv.buckets[bucket.Bucket]; !ok {
			return errors.New("bucket already deleted")
		}
		delete(kv.buckets, bucket.Bucket)
	}
	return nil
}

func (kv *Service) ListBuckets() (*protos.BucketListResponse, error) {
	buckets := &protos.BucketListResponse{Results: []string{}}
	if config.Config.PersistentStorageEnabled {
		if allBuckets, err := kv.dbConnection.GetAllBuckets(); err != nil {
			logger.ErrorLogger.Println(err)
		} else {
			for _, bucket := range allBuckets {
				buckets.Results = append(buckets.Results, bucket.Bucket)
			}
		}
	} else {
		kv.bucketsLock.RLock()
		defer kv.bucketsLock.RUnlock()
		buckets.Results = append(buckets.Results, maps.Keys(kv.buckets)...)
	}
	return buckets, nil
}

func (kv *Service) LookupBucket(bucket *protos.Bucket) error {
	if config.Config.PersistentStorageEnabled {
		return kv.dbConnection.LookupBucket(bucket)
	} else {
		kv.bucketsLock.RLock()
		defer kv.bucketsLock.RUnlock()
		if _, ok := kv.buckets[bucket.Bucket]; !ok {
			return errors.New("bucket does not exist")
		}
		return nil
	}
}

func (kv *Service) LookupBucketByName(bucketName string) error {
	if config.Config.PersistentStorageEnabled {
		return kv.dbConnection.LookupBucket(&protos.Bucket{Bucket: bucketName})
	} else {
		kv.bucketsLock.RLock()
		defer kv.bucketsLock.RUnlock()
		if _, ok := kv.buckets[bucketName]; !ok {
			return errors.New("bucket does not exist")
		}
		return nil
	}
}

func (kv *Service) CreateObject(object *protos.Object) error {
	if config.Config.PersistentStorageEnabled {
		kv.dbConnection.ObjectChan <- &db.OperationParams{
			Object: object,
			Type:   db.PUT,
		}
	} else {
		bucket, _ := kv.NewBucketIfNotExist(object.Id)
		kv.traverseCreateObject(bucket, object)
	}
	return nil
}

func (kv *Service) UpdateObject(object *protos.Object) error {
	if config.Config.PersistentStorageEnabled {
		kv.dbConnection.ObjectChan <- &db.OperationParams{
			Object: object,
			Type:   db.PUT,
		}
	} else {
		bucket, _ := kv.NewBucketIfNotExist(object.Id)
		kv.traverseCreateObject(bucket, object)
	}
	return nil
}

func (kv *Service) DeleteObject(objectID *protos.ObjectID) error {
	if config.Config.PersistentStorageEnabled {
		kv.dbConnection.ObjectChan <- &db.OperationParams{
			Object: &protos.Object{
				Id: objectID,
			},
			Type: db.DELETE,
		}
	} else {
		bucket, _ := kv.NewBucketIfNotExist(objectID)
		kv.traverseDeleteObject(bucket, objectID)
	}
	return nil
}

func (kv *Service) DeleteObjectPrefix(objectID *protos.ObjectID) ([]*protos.Object, error) {
	var deletedObjects []*protos.Object
	if config.Config.PersistentStorageEnabled {
		if allObjectsPrefix, err := kv.dbConnection.GetAllObjectsPrefix(objectID); err != nil {
			logger.ErrorLogger.Println(err)
		} else {
			deletedObjects = append(deletedObjects, allObjectsPrefix...)
			for _, object := range allObjectsPrefix {
				kv.dbConnection.ObjectChan <- &db.OperationParams{
					Object: object,
					Type:   db.DELETE,
				}
			}
		}
	} else {
		bucket, _ := kv.NewBucketIfNotExist(objectID)
		deletedObjects = kv.traverseDeleteObjectPrefix(bucket, objectID)
	}
	return deletedObjects, nil
}

func (kv *Service) LookupObject(objectID *protos.ObjectID) (*protos.ObjectResponse, error) {
	if config.Config.PersistentStorageEnabled {
		if object, err := kv.dbConnection.GetObjectWithObjectID(objectID); err != nil {
			return nil, errors.New("object does not exist")
		} else {
			return &protos.ObjectResponse{
				Result: object,
			}, nil
		}
	} else {
		bucket, _ := kv.NewBucketIfNotExist(objectID)
		if object, ok := kv.lookUpObject(bucket, objectID); !ok {
			return nil, errors.New("object does not exist")
		} else {
			return &protos.ObjectResponse{
				Result: object,
			}, nil
		}
	}
}

func (kv *Service) ListObjects(objectListRequest *protos.ObjectListRequest) (*protos.ObjectListResponse, error) {
	objects := &protos.ObjectListResponse{Results: []*protos.Object{}, CommonPrefixes: []string{}}
	if objectListRequest.Prefix == nil || len(objectListRequest.Prefix.Bucket) == 0 {
		logger.InfoLogger.Println("bucket not set")
		return objects, nil
	}
	var delimiter string
	if objectListRequest.Delimiter != nil && *objectListRequest.Delimiter != 0 {
		delimiter = string(*objectListRequest.Delimiter)
		if delimiter != db.CommonDelimiter {
			logger.InfoLogger.Println("this delimiter is not supported:", delimiter)
			return objects, nil
		}
	}
	if config.Config.PersistentStorageEnabled {
		if len(delimiter) == 0 {
			if allObjectsPrefix, err := kv.dbConnection.GetAllObjectsPrefix(objectListRequest.Prefix); err != nil {
				logger.ErrorLogger.Println(err)
			} else {
				objects.Results = append(objects.Results, allObjectsPrefix...)
			}
		} else {
			allObjectsPrefixIDs := []string{}
			var err error
			if allObjectsPrefixIDs, err = kv.dbConnection.GetAllObjectsIDsPrefix(objectListRequest.Prefix); err != nil {
				logger.ErrorLogger.Println(err)
			}
			filteredIDs, filteredPrefix := kv.filterIDsAndCommonPrefix(allObjectsPrefixIDs, objectListRequest.Prefix)
			objects.CommonPrefixes = filteredPrefix
			if len(objectListRequest.Prefix.Key) == 0 {
				for _, objectID := range filteredIDs {
					if object, err := kv.dbConnection.GetObjectWithStringID(objectID); err != nil {
						logger.ErrorLogger.Println(err)
					} else {
						objects.Results = append(objects.Results, object)
					}
				}
			} else {
				if allObjectsPrefix, err := kv.dbConnection.GetAllObjectsPrefix(objectListRequest.Prefix); err != nil {
					logger.ErrorLogger.Println(err)
				} else {
					objects.Results = append(objects.Results, allObjectsPrefix...)
				}
			}
		}
	} else {
		bucket, _ := kv.NewBucketIfNotExist(objectListRequest.Prefix)
		objects = kv.listObjects(bucket, objectListRequest.Prefix, delimiter)
	}
	return objects, nil
}
