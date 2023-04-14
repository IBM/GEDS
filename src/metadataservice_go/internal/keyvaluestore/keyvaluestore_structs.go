package keyvaluestore

import (
	"github.com/IBM/gedsmds/internal/keyvaluestore/db"
	"github.com/IBM/gedsmds/protos"
	"sync"
)

const commonDelimiter = "/"

type Service struct {
	dbConnection *db.Operations

	objectStoreConfigsLock *sync.RWMutex
	objectStoreConfigs     map[string]*protos.ObjectStoreConfig

	bucketsLock *sync.RWMutex
	buckets     map[string]*Bucket
}

type Object struct {
	path   []string
	object *protos.Object
}

type NestedObjects struct {
	objects          map[string]*Object
	currentDirectory string
	childDirectories map[string]*NestedObjects
}

type Bucket struct {
	objectsLock   *sync.RWMutex
	objects       map[string]*Object
	nestedObjects *NestedObjects
	bucket        *protos.Bucket
}
