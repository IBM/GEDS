/**
 * Copyright 2023- Pezhman Nasirifard. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

package db

import (
	"github.com/IBM/gedsmds/protos"
	"github.com/syndtr/goleveldb/leveldb"
)

const objectStoreConfigLocation = "./data/state_object_store_configs"
const bucketLocation = "./data/state_buckets"
const objectLocation = "./data/state_objects"

const channelBufferSize = 10000

const (
	PUT    int = 1
	DELETE     = 2
)

type OperationParams struct {
	ObjectStoreConfig *protos.ObjectStoreConfig
	Bucket            *protos.Bucket
	Object            *protos.Object
	Type              int
}

type Operations struct {
	dbObjectStoreConfig *leveldb.DB
	dbBucket            *leveldb.DB
	dbObject            *leveldb.DB

	ObjectStoreConfigChan chan *OperationParams
	BucketChan            chan *OperationParams
	ObjectChan            chan *OperationParams
}
