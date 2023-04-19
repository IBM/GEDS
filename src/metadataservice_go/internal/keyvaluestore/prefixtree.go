package keyvaluestore

import "github.com/IBM/gedsmds/protos"

func makeNewPrefixTree(thisDirectory string) *PrefixTree {
	return &PrefixTree{
		objectsInThisDirectory:           map[string]*protos.Object{},
		thisDirectory:                    thisDirectory,
		childDirectories:                 map[string]*PrefixTree{},
		objectsInThisAndChildDirectories: map[string]*protos.Object{},
	}
}

func (kv *Service) traverseCreateObject(bucket *Bucket, object *protos.Object) {
	nestedPath := kv.getNestedPath(object.Id)
	nestedDirs := len(nestedPath) - 1
	bucket.objectsLock.Lock()
	currentNode := bucket.nestedDirectories
	currentNode.objectsInThisAndChildDirectories[object.Id.Key] = object
	for i := 0; i < nestedDirs; i++ {
		if childNode, ok := currentNode.childDirectories[nestedPath[i]]; ok {
			currentNode = childNode
		} else {
			currentNode.childDirectories[nestedPath[i]] = makeNewPrefixTree(nestedPath[i])
			currentNode = currentNode.childDirectories[nestedPath[i]]
		}
		currentNode.objectsInThisAndChildDirectories[object.Id.Key] = object
	}
	currentNode.objectsInThisDirectory[object.Id.Key] = object
	bucket.objectsLock.Unlock()
}

func (kv *Service) lookUpObject(bucket *Bucket, objectId *protos.ObjectID) (*protos.Object, bool) {
	nestedPath := kv.getNestedPath(objectId)
	nestedDirs := len(nestedPath) - 1
	bucket.objectsLock.RLock()
	currentNode := bucket.nestedDirectories
	for i := 0; i < nestedDirs; i++ {
		if childNode, ok := currentNode.childDirectories[nestedPath[i]]; ok {
			currentNode = childNode
		} else {
			break
		}
	}
	object, ok := currentNode.objectsInThisDirectory[objectId.Key]
	bucket.objectsLock.RUnlock()
	return object, ok
}

func (kv *Service) traverseListObjects(bucket *Bucket, objectId *protos.ObjectID, getCommonPrefix bool) ([]*protos.Object, []string) {
	objectListed := []*protos.Object{}
	prefixes := []string{}
	var longestPath string
	nestedPath := kv.getNestedPath(objectId)
	nestedDirs := len(nestedPath)
	bucket.objectsLock.RLock()
	defer bucket.objectsLock.RUnlock()
	currentNode := bucket.nestedDirectories
	for i := 0; i < nestedDirs; i++ {
		if childNode, ok := currentNode.childDirectories[nestedPath[i]]; ok {
			currentNode = childNode
			if getCommonPrefix {
				longestPath += currentNode.thisDirectory + commonDelimiter
			}
		} else {
			return objectListed, prefixes
		}
	}
	for _, object := range currentNode.objectsInThisAndChildDirectories {
		objectListed = append(objectListed, object)
	}
	if getCommonPrefix {
		for prefix := range currentNode.childDirectories {
			prefixes = append(prefixes, longestPath+prefix)
		}
	}
	return objectListed, prefixes
}

func (kv *Service) listObjects(bucket *Bucket, objectId *protos.ObjectID, delimiter string) *protos.ObjectListResponse {
	objects := &protos.ObjectListResponse{Results: []*protos.Object{}, CommonPrefixes: []string{}}
	if len(delimiter) == 0 {
		objects.Results, _ = kv.traverseListObjects(bucket, objectId, false)
	} else {
		if len(objectId.Key) == 0 {
			bucket.objectsLock.RLock()
			rootDir := bucket.nestedDirectories
			for _, object := range rootDir.objectsInThisDirectory {
				objects.Results = append(objects.Results, object)
			}
			for commonPrefix := range rootDir.childDirectories {
				objects.CommonPrefixes = append(objects.CommonPrefixes, commonPrefix+commonDelimiter)
			}
			bucket.objectsLock.RUnlock()
		} else {
			objects.Results, objects.CommonPrefixes = kv.traverseListObjects(bucket, objectId, true)
		}
	}
	return objects
}

func (kv *Service) traverseDeleteObject(bucket *Bucket, objectId *protos.ObjectID) {
	nestedPath := kv.getNestedPath(objectId)
	nestedDirs := len(nestedPath) - 1
	bucket.objectsLock.Lock()
	currentNode := bucket.nestedDirectories
	delete(currentNode.objectsInThisAndChildDirectories, objectId.Key)
	for i := 0; i < nestedDirs; i++ {
		if childNode, ok := currentNode.childDirectories[nestedPath[i]]; ok {
			currentNode = childNode
		} else {
			break
		}
		delete(currentNode.objectsInThisAndChildDirectories, objectId.Key)
	}
	delete(currentNode.objectsInThisDirectory, objectId.Key)
	bucket.objectsLock.Unlock()
}

func (kv *Service) traverseDeleteObjectPrefix(bucket *Bucket, objectId *protos.ObjectID) []*protos.Object {
	deletedObjects := []*protos.Object{}
	nestedPath := kv.getNestedPath(objectId)
	nestedDirs := len(nestedPath)
	bucket.objectsLock.Lock()
	currentNode := bucket.nestedDirectories
	for i := 0; i < nestedDirs; i++ {
		if childNode, ok := currentNode.childDirectories[nestedPath[i]]; ok {
			currentNode = childNode
		} else {
			break
		}
	}
	for _, object := range currentNode.objectsInThisAndChildDirectories {
		deletedObjects = append(deletedObjects, object)
	}
	currentNode = bucket.nestedDirectories
	for _, toBeDeletedObject := range deletedObjects {
		delete(currentNode.objectsInThisDirectory, toBeDeletedObject.Id.Key)
		delete(currentNode.objectsInThisAndChildDirectories, toBeDeletedObject.Id.Key)
	}
	for i := 0; i < nestedDirs; i++ {
		if childNode, ok := currentNode.childDirectories[nestedPath[i]]; ok {
			currentNode = childNode
		} else {
			break
		}
		for _, toBeDeletedObject := range deletedObjects {
			delete(currentNode.objectsInThisDirectory, toBeDeletedObject.Id.Key)
			delete(currentNode.objectsInThisAndChildDirectories, toBeDeletedObject.Id.Key)
		}
	}
	bucket.objectsLock.Unlock()
	return deletedObjects
}
