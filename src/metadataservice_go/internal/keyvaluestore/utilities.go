/**
 * Copyright 2023- Pezhman Nasirifard. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

package keyvaluestore

import (
	"context"
	"github.com/IBM/gedsmds/internal/keyvaluestore/db"
	"github.com/IBM/gedsmds/internal/logger"
	"github.com/IBM/gedsmds/protos"
	"github.com/aws/aws-sdk-go-v2/aws"
	"github.com/aws/aws-sdk-go-v2/credentials"
	"github.com/aws/aws-sdk-go-v2/service/s3"
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

func (kv *Service) populateKVSFromS3(objectStore *protos.ObjectStoreConfig) {
	const defaultEndpointOptions = "custom"
	customResolver := aws.EndpointResolverWithOptionsFunc(func(service, region string,
		options ...interface{}) (aws.Endpoint, error) {
		return aws.Endpoint{
			PartitionID:       defaultEndpointOptions,
			SigningRegion:     defaultEndpointOptions,
			HostnameImmutable: true,
			URL:               objectStore.EndpointUrl,
		}, nil
	})
	cfg := aws.NewConfig()
	cfg.EndpointResolverWithOptions = customResolver
	cfg.Credentials = credentials.NewStaticCredentialsProvider(objectStore.AccessKey, objectStore.SecretKey, "")
	client := s3.NewFromConfig(*cfg)
	p := s3.NewListObjectsV2Paginator(client, &s3.ListObjectsV2Input{Bucket: &objectStore.Bucket},
		func(o *s3.ListObjectsV2PaginatorOptions) { o.Limit = 1000 })
	for p.HasMorePages() {
		page, err := p.NextPage(context.TODO())
		if err != nil {
			logger.ErrorLogger.Println("error with retrieving the pages from s3: ", err)
			break
		}
		for _, s3Object := range page.Contents {
			object := &protos.Object{
				Id: &protos.ObjectID{
					Bucket: objectStore.Bucket,
					Key:    *s3Object.Key,
				},
				Info: &protos.ObjectInfo{
					Size:         uint64(s3Object.Size),
					SealedOffset: uint64(s3Object.Size),
					Location:     "s3://" + objectStore.Bucket + "/" + *s3Object.Key,
				},
			}
			if err = kv.CreateObject(object); err != nil {
				logger.ErrorLogger.Println("error with creating object from s3: ", err)
			}
		}
	}
}
