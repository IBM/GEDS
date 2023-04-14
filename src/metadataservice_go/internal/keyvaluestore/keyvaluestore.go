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
	return kvStore
}

func (kv *Service) NewBucketIfNotExist(objectID *protos.ObjectID) (*Bucket, bool) {
	kv.bucketsLock.Lock()
	defer kv.bucketsLock.Unlock()
	if bucket, ok := kv.buckets[objectID.Bucket]; !ok {
		kv.buckets[objectID.Bucket] = &Bucket{
			bucket:      &protos.Bucket{Bucket: objectID.Bucket},
			objectsLock: &sync.RWMutex{},
			objects:     map[string]*Object{},
			nestedObjects: &NestedObjects{
				objects:          map[string]*Object{},
				currentDirectory: "root",
				childDirectories: map[string]*NestedObjects{},
			},
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
		newObject := &Object{object: object, path: kv.getNestedPath(object.Id)}
		bucket, _ := kv.NewBucketIfNotExist(object.Id)
		bucket.objectsLock.Lock()
		bucket.objects[object.Id.Key] = newObject
		bucket.objectsLock.Unlock()

		nestedPath := kv.getNestedPath(object.Id)
		bucket.objectsLock.Lock()
		currentNode := bucket.nestedObjects
		for i := 0; i < len(nestedPath)-1; i++ {
			if childNode, ok := currentNode.childDirectories[nestedPath[i]]; ok {
				currentNode = childNode
			} else {
				currentNode.childDirectories[nestedPath[i]] = &NestedObjects{
					objects:          map[string]*Object{},
					currentDirectory: nestedPath[i],
					childDirectories: map[string]*NestedObjects{},
				}
				currentNode = currentNode.childDirectories[nestedPath[i]]
			}
		}
		currentNode.objects[nestedPath[len(nestedPath)-1]] = newObject
		bucket.objectsLock.Unlock()
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
		newObject := &Object{object: object, path: kv.getNestedPath(object.Id)}
		kv.bucketsLock.Lock()
		bucket, _ := kv.NewBucketIfNotExist(object.Id)
		bucket.objectsLock.Lock()
		bucket.objects[object.Id.Key] = newObject
		bucket.objectsLock.Unlock()

		nestedPath := kv.getNestedPath(object.Id)
		bucket.objectsLock.Lock()
		currentNode := bucket.nestedObjects
		for i := 0; i < len(nestedPath)-1; i++ {
			if childNode, ok := currentNode.childDirectories[nestedPath[i]]; ok {
				currentNode = childNode
			} else {
				currentNode.childDirectories[nestedPath[i]] = &NestedObjects{
					objects:          map[string]*Object{},
					currentDirectory: nestedPath[i],
					childDirectories: map[string]*NestedObjects{},
				}
				currentNode = currentNode.childDirectories[nestedPath[i]]
			}
		}
		currentNode.objects[nestedPath[len(nestedPath)-1]] = newObject
		bucket.objectsLock.Unlock()
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
		bucket.objectsLock.Lock()
		delete(bucket.objects, objectID.Key)
		bucket.objectsLock.Unlock()
	}
	return nil
}

func (kv *Service) DeleteObjectPrefix(objectID *protos.ObjectID) ([]*protos.Object, error) {
	var deletedObjects []*protos.Object
	if config.Config.PersistentStorageEnabled {
		// !!!!!!!!!!
		for _, object := range deletedObjects {
			kv.dbConnection.ObjectChan <- &db.OperationParams{
				Object: object,
				Type:   db.DELETE,
			}
		}
	} else {
		bucket, _ := kv.NewBucketIfNotExist(objectID)
		bucket.objectsLock.Lock()
		for key := range bucket.objects {
			if strings.HasPrefix(key, objectID.Key) {
				delete(bucket.objects, objectID.Key)
			}
		}
		bucket.objectsLock.Unlock()
	}
	return deletedObjects, nil
}

func (kv *Service) LookupObject(objectID *protos.ObjectID) (*protos.ObjectResponse, error) {
	if config.Config.PersistentStorageEnabled {
		if object, err := kv.dbConnection.GetObject(&protos.Object{Id: objectID}); err != nil {
			return nil, errors.New("object does not exist")
		} else {
			return &protos.ObjectResponse{
				Result: object,
			}, nil
		}
	} else {
		bucket, _ := kv.NewBucketIfNotExist(objectID)
		bucket.objectsLock.RLock()
		defer bucket.objectsLock.RUnlock()
		if object, ok := bucket.objects[objectID.Key]; !ok {
			return nil, errors.New("object does not exist")
		} else {
			return &protos.ObjectResponse{
				Result: object.object,
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
	}
	tempCommonPrefixes := map[string]bool{}

	if config.Config.PersistentStorageEnabled {

	} else {
		bucket, _ := kv.NewBucketIfNotExist(objectListRequest.Prefix)
		// needs to be optimized
		if len(delimiter) == 0 {
			bucket.objectsLock.RLock()
			for key, object := range bucket.objects {
				if strings.HasPrefix(key, objectListRequest.Prefix.Key) {
					objects.Results = append(objects.Results, object.object)
				}
			}
			bucket.objectsLock.RUnlock()
		} else {
			if len(objectListRequest.Prefix.Key) == 0 {
				bucket.objectsLock.RLock()
				for _, object := range bucket.objects {
					if len(object.path) == 1 {
						objects.Results = append(objects.Results, object.object)
					} else if len(object.path) > 1 {
						tempCommonPrefixes[object.path[0]] = true
					}
				}
				bucket.objectsLock.RUnlock()
			} else {
				prefixLength := len(strings.Split(objectListRequest.Prefix.Key, delimiter)) + 1

				bucket.objectsLock.RLock()
				for key, object := range bucket.objects {
					if strings.HasPrefix(key, objectListRequest.Prefix.Key) {
						objects.Results = append(objects.Results, object.object)
						if len(object.path) == prefixLength {
							tempCommonPrefixes[strings.Join(object.path[:prefixLength-1], delimiter)] = true
						}
					}
				}
				bucket.objectsLock.RUnlock()
			}
		}
		if len(tempCommonPrefixes) > 0 {
			for commonPrefix := range tempCommonPrefixes {
				objects.CommonPrefixes = append(objects.CommonPrefixes, commonPrefix+commonDelimiter)
			}
		}
	}

	return objects, nil
}
