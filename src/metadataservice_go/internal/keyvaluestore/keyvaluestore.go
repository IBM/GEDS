package keyvaluestore

import (
	"errors"
	"github.com/IBM/gedsmds/internal/config"
	"github.com/IBM/gedsmds/internal/keyvaluestore/db"
	"github.com/IBM/gedsmds/internal/logger"
	"github.com/IBM/gedsmds/protos"
	"golang.org/x/exp/maps"
	"strings"
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
	go kvStore.populateCache()
	return kvStore
}

func (kv *Service) NewBucketIfNotExist(objectID *protos.ObjectID) {
	if _, ok := kv.buckets[objectID.Bucket]; !ok {
		kv.buckets[objectID.Bucket] = &Bucket{
			bucket:      &protos.Bucket{Bucket: objectID.Bucket},
			objectsLock: &sync.RWMutex{},
			objects:     map[string]*Object{},
		}
	}
}

func (kv *Service) populateCache() {
	if config.Config.PersistentStorageEnabled && config.Config.RepopulateCacheEnabled {
		go kv.populateObjectStoreConfig()
		go kv.populateBuckets()
	}
}

func (kv *Service) populateObjectStoreConfig() {
	if allObjectStoreConfig, err := kv.dbConnection.GetAllObjectStoreConfig(); err != nil {
		logger.ErrorLogger.Println(err)
	} else {
		kv.objectStoreConfigsLock.Lock()
		for _, objectStoreConfig := range allObjectStoreConfig {
			kv.objectStoreConfigs[objectStoreConfig.Bucket] = objectStoreConfig
		}
		kv.objectStoreConfigsLock.Unlock()
	}
}

func (kv *Service) populateBuckets() {
	if allBuckets, err := kv.dbConnection.GetAllBuckets(); err != nil {
		logger.ErrorLogger.Println(err)
	} else {
		kv.bucketsLock.Lock()
		for _, bucket := range allBuckets {
			kv.NewBucketIfNotExist(&protos.ObjectID{Bucket: bucket.Bucket})
		}
		kv.bucketsLock.Unlock()
	}
	if allObjects, err := kv.dbConnection.GetAllObjects(); err != nil {
		logger.ErrorLogger.Println(err)
	} else {
		kv.bucketsLock.Lock()
		for _, object := range allObjects {
			kv.NewBucketIfNotExist(object.Id)
			kv.buckets[object.Id.Bucket].objects[object.Id.Key] = &Object{object: object, path: kv.getNestedPath(object.Id)}
		}
		kv.bucketsLock.Unlock()
	}
}

func (kv *Service) RegisterObjectStore(objectStore *protos.ObjectStoreConfig) error {
	kv.objectStoreConfigsLock.Lock()
	defer kv.objectStoreConfigsLock.Unlock()
	if _, ok := kv.objectStoreConfigs[objectStore.Bucket]; ok {
		return errors.New("config already exists")
	}
	kv.objectStoreConfigs[objectStore.Bucket] = objectStore
	if config.Config.PersistentStorageEnabled {
		kv.dbConnection.ObjectStoreConfigChan <- &db.OperationParams{
			ObjectStoreConfig: objectStore,
			Type:              db.PUT,
		}
	}
	return nil
}

func (kv *Service) ListObjectStores() (*protos.AvailableObjectStoreConfigs, error) {
	kv.objectStoreConfigsLock.RLock()
	defer kv.objectStoreConfigsLock.RUnlock()
	mappings := &protos.AvailableObjectStoreConfigs{Mappings: []*protos.ObjectStoreConfig{}}
	for _, objectStoreConfig := range kv.objectStoreConfigs {
		mappings.Mappings = append(mappings.Mappings, objectStoreConfig)
	}
	return mappings, nil
}

func (kv *Service) CreateBucket(bucket *protos.Bucket) error {
	kv.bucketsLock.Lock()
	defer kv.bucketsLock.Unlock()
	if _, ok := kv.buckets[bucket.Bucket]; ok {
		return errors.New("bucket already exists")
	}
	kv.NewBucketIfNotExist(&protos.ObjectID{Bucket: bucket.Bucket})
	if config.Config.PersistentStorageEnabled {
		kv.dbConnection.BucketChan <- &db.OperationParams{
			Bucket: bucket,
			Type:   db.PUT,
		}
	}
	return nil
}

func (kv *Service) DeleteBucket(bucket *protos.Bucket) error {
	kv.bucketsLock.Lock()
	defer kv.bucketsLock.Unlock()
	if _, ok := kv.buckets[bucket.Bucket]; !ok {
		return errors.New("bucket already deleted")
	}
	delete(kv.buckets, bucket.Bucket)
	if config.Config.PersistentStorageEnabled {
		kv.dbConnection.BucketChan <- &db.OperationParams{
			Bucket: bucket,
			Type:   db.DELETE,
		}
	}
	// delete objects in bucket
	return nil
}

func (kv *Service) ListBuckets() (*protos.BucketListResponse, error) {
	kv.bucketsLock.RLock()
	defer kv.bucketsLock.RUnlock()
	buckets := &protos.BucketListResponse{Results: []string{}}
	buckets.Results = append(buckets.Results, maps.Keys(kv.buckets)...)
	return buckets, nil
}

func (kv *Service) LookupBucket(bucket *protos.Bucket) error {
	kv.bucketsLock.RLock()
	defer kv.bucketsLock.RUnlock()
	if _, ok := kv.buckets[bucket.Bucket]; !ok {
		return errors.New("bucket does not exist")
	}
	return nil
}

func (kv *Service) LookupBucketByName(bucketName string) error {
	kv.bucketsLock.RLock()
	defer kv.bucketsLock.RUnlock()
	if _, ok := kv.buckets[bucketName]; !ok {
		return errors.New("bucket does not exist")
	}
	return nil
}

func (kv *Service) CreateObject(object *protos.Object) error {
	newObject := &Object{object: object, path: kv.getNestedPath(object.Id)}
	kv.bucketsLock.Lock()
	kv.NewBucketIfNotExist(object.Id)
	kv.buckets[object.Id.Bucket].objects[object.Id.Key] = newObject
	kv.bucketsLock.Unlock()
	logger.InfoLogger.Println(object)
	if config.Config.PersistentStorageEnabled {
		kv.dbConnection.ObjectChan <- &db.OperationParams{
			Object: object,
			Type:   db.PUT,
		}
	}
	return nil
}

func (kv *Service) UpdateObject(object *protos.Object) error {
	newObject := &Object{object: object, path: kv.getNestedPath(object.Id)}
	kv.bucketsLock.Lock()
	kv.NewBucketIfNotExist(object.Id)
	kv.buckets[object.Id.Bucket].objects[object.Id.Key] = newObject
	kv.bucketsLock.Unlock()
	if config.Config.PersistentStorageEnabled {
		kv.dbConnection.ObjectChan <- &db.OperationParams{
			Object: object,
			Type:   db.PUT,
		}
	}
	return nil
}

func (kv *Service) CreateOrUpdateObject(object *protos.Object) error {
	newObject := &Object{object: object, path: kv.getNestedPath(object.Id)}
	kv.bucketsLock.Lock()
	kv.NewBucketIfNotExist(object.Id)
	kv.buckets[object.Id.Bucket].objects[object.Id.Key] = newObject
	kv.bucketsLock.Unlock()
	if config.Config.PersistentStorageEnabled {
		kv.dbConnection.ObjectChan <- &db.OperationParams{
			Object: object,
			Type:   db.PUT,
		}
	}
	return nil
}

func (kv *Service) DeleteObject(objectID *protos.ObjectID) error {
	kv.bucketsLock.Lock()
	if _, ok := kv.buckets[objectID.Bucket]; ok {
		delete(kv.buckets[objectID.Bucket].objects, objectID.Key)
	}
	kv.bucketsLock.Unlock()
	if config.Config.PersistentStorageEnabled {
		kv.dbConnection.ObjectChan <- &db.OperationParams{
			Object: &protos.Object{
				Id: objectID,
			},
			Type: db.DELETE,
		}
	}
	return nil
}

func (kv *Service) DeleteObjectPrefix(objectID *protos.ObjectID) ([]*protos.Object, error) {
	var objects []*protos.Object
	// this will be slow, needs to be optimized
	// needs the cache to be repopulated
	kv.bucketsLock.Lock()
	if _, ok := kv.buckets[objectID.Bucket]; ok {
		for key, object := range kv.buckets[objectID.Bucket].objects {
			if strings.HasPrefix(key, objectID.Key) {
				objects = append(objects, object.object)
				delete(kv.buckets, objectID.Key)
			}
		}
	}
	kv.bucketsLock.Unlock()
	if config.Config.PersistentStorageEnabled {
		for _, object := range objects {
			kv.dbConnection.ObjectChan <- &db.OperationParams{
				Object: object,
				Type:   db.DELETE,
			}
		}
	}
	return objects, nil
}

func (kv *Service) LookupObject(objectID *protos.ObjectID) (*protos.ObjectResponse, error) {
	kv.bucketsLock.RLock()
	defer kv.bucketsLock.RUnlock()
	if kv.buckets[objectID.Bucket] == nil {
		return nil, errors.New("object does not exist")
	}
	if _, ok := kv.buckets[objectID.Bucket].objects[objectID.Key]; !ok {
		return nil, errors.New("object does not exist")
	}
	return &protos.ObjectResponse{
		Result: kv.buckets[objectID.Bucket].objects[objectID.Key].object,
	}, nil
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
	}
	tempCommonPrefixes := map[string]bool{}
	// needs to be optimized
	if len(delimiter) == 0 {
		kv.bucketsLock.RLock()
		if _, ok := kv.buckets[objectListRequest.Prefix.Bucket]; ok {
			for key, object := range kv.buckets[objectListRequest.Prefix.Bucket].objects {
				if strings.HasPrefix(key, objectListRequest.Prefix.Key) {
					objects.Results = append(objects.Results, object.object)
				}
			}
		}
		kv.bucketsLock.RUnlock()
	} else {
		if len(objectListRequest.Prefix.Key) == 0 {
			kv.bucketsLock.RLock()
			if _, ok := kv.buckets[objectListRequest.Prefix.Bucket]; ok {
				for _, object := range kv.buckets[objectListRequest.Prefix.Bucket].objects {
					if len(object.path) == 1 {
						objects.Results = append(objects.Results, object.object)
					} else if len(object.path) > 1 {
						tempCommonPrefixes[object.path[0]] = true
					}
				}
			}
			kv.bucketsLock.RUnlock()
		} else {
			prefixLength := len(strings.Split(objectListRequest.Prefix.Key, delimiter)) + 1
			kv.bucketsLock.RLock()
			if _, ok := kv.buckets[objectListRequest.Prefix.Bucket]; ok {
				for key, object := range kv.buckets[objectListRequest.Prefix.Bucket].objects {
					if strings.HasPrefix(key, objectListRequest.Prefix.Key) {
						objects.Results = append(objects.Results, object.object)
						if len(object.path) == prefixLength {
							tempCommonPrefixes[strings.Join(object.path[:prefixLength-1], delimiter)] = true
						}
					}
				}
			}
			kv.bucketsLock.RUnlock()
		}
	}
	if len(tempCommonPrefixes) > 0 {
		for commonPrefix := range tempCommonPrefixes {
			objects.CommonPrefixes = append(objects.CommonPrefixes, commonPrefix+commonDelimiter)
		}
	}

	return objects, nil
}
