package keyvaluestore

import (
	"github.com/IBM/gedsmds/internal/keyvaluestore/db"
	"github.com/IBM/gedsmds/protos"
	"sync"
)

type Service struct {
	dbConnection *db.Operations

	objectStoreConfigsLock *sync.RWMutex
	objectStoreConfigs     map[string]*protos.ObjectStoreConfig

	bucketsLock *sync.RWMutex
	buckets     map[string]*Bucket
}

type PrefixTree struct {
	objectsInThisDirectory           map[string]*protos.Object
	thisDirectory                    string
	childDirectories                 map[string]*PrefixTree
	objectsInThisAndChildDirectories map[string]*protos.Object
}

type Bucket struct {
	objectsLock       *sync.RWMutex
	nestedDirectories *PrefixTree
	bucket            *protos.Bucket
}
