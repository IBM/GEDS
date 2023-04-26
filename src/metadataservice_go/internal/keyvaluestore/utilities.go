/**
 * Copyright 2023- Pezhman Nasirifard. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

package keyvaluestore

import (
	"github.com/IBM/gedsmds/internal/keyvaluestore/db"
	"github.com/IBM/gedsmds/protos"
	"strings"
	"sync"
)

func (kv *Service) getNestedPath(objectID *protos.ObjectID) []string {
	return kv.getNestedPathWithKey(objectID.Key)
}

func (kv *Service) getNestedPathWithKey(objectID string) []string {
	nestedPath := strings.Split(strings.TrimSpace(objectID), db.CommonDelimiter)
	lenNestedPath := len(nestedPath)
	if lenNestedPath > 0 && len(nestedPath[lenNestedPath-1]) == 0 {
		nestedPath = nestedPath[:lenNestedPath-1]
	}
	return nestedPath
}

func (kv *Service) newBucketIfNotExist(objectID *protos.ObjectID) (*Bucket, bool) {
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

func (kv *Service) filterIDsAndCommonPrefix(objectsIDs []string, objectQuery *protos.ObjectID) ([]string, []string) {
	filteredIDs := []string{}
	filteredPrefix := []string{}
	tempCommonPrefixes := map[string]bool{}
	prefixLength := len(kv.getNestedPath(objectQuery)) + 2
	for _, objectID := range objectsIDs {
		keyParts := kv.getNestedPathWithKey(objectID)
		if len(objectQuery.Key) == 0 {
			if len(keyParts) == 2 {
				filteredIDs = append(filteredIDs, objectID)
			} else if len(keyParts) > 2 {
				tempCommonPrefixes[keyParts[1]] = true
			}
		} else {
			if len(keyParts) >= prefixLength {
				tempCommonPrefixes[strings.Join(keyParts[1:prefixLength], db.CommonDelimiter)] = true
			}
		}
	}

	if len(tempCommonPrefixes) > 0 {
		for commonPrefix := range tempCommonPrefixes {
			filteredPrefix = append(filteredPrefix, commonPrefix+db.CommonDelimiter)
		}
	}
	return filteredIDs, filteredPrefix
}
