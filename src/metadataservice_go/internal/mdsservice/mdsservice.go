/**
 * Copyright 2023- Pezhman Nasirifard. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

package mdsservice

import (
	"context"
	"github.com/IBM/gedsmds/internal/config"
	"github.com/IBM/gedsmds/internal/logger"
	"github.com/IBM/gedsmds/internal/mdsprocessor"
	"github.com/IBM/gedsmds/internal/prommetrics"
	"github.com/IBM/gedsmds/protos"
)

func NewService(metrics *prommetrics.Metrics) *Service {
	if config.Config.PubSubEnabled {
		logger.InfoLogger.Println("publish/subscribe is enabled")
	} else {
		logger.InfoLogger.Println("publish/subscribe is disabled")
	}
	if config.Config.PersistentStorageEnabled {
		logger.InfoLogger.Println("persistent storage enabled")
	} else {
		logger.InfoLogger.Println("in-memory storage enabled")
	}
	return &Service{
		processor: mdsprocessor.InitService(metrics),
		metrics:   metrics,
	}
}

func (s *Service) GetConnectionInformation(ctx context.Context,
	_ *protos.EmptyParams) (*protos.ConnectionInformation, error) {
	if address, err := s.processor.GetClientConnectionInformation(ctx); err != nil {
		return &protos.ConnectionInformation{}, err
	} else {
		logger.InfoLogger.Println("sending connection info: ", address)
		return &protos.ConnectionInformation{RemoteAddress: address}, nil
	}
}

func (s *Service) RegisterObjectStore(_ context.Context,
	objectStore *protos.ObjectStoreConfig) (*protos.StatusResponse, error) {
	logger.InfoLogger.Println("register objectStore", objectStore)
	if err := s.processor.RegisterObjectStore(objectStore); err != nil {
		// It should return status code already exist
		return &protos.StatusResponse{Code: protos.StatusCode_OK}, nil
	}
	return &protos.StatusResponse{Code: protos.StatusCode_OK}, nil
}

func (s *Service) ListObjectStores(_ context.Context,
	_ *protos.EmptyParams) (*protos.AvailableObjectStoreConfigs, error) {
	logger.InfoLogger.Println("list object stores")
	return s.processor.ListObjectStores()
}

func (s *Service) CreateBucket(_ context.Context, bucket *protos.Bucket) (*protos.StatusResponse, error) {
	logger.InfoLogger.Println("create bucket", bucket)
	if err := s.processor.CreateBucket(bucket); err != nil {
		return &protos.StatusResponse{Code: protos.StatusCode_ALREADY_EXISTS}, nil
	}
	return &protos.StatusResponse{Code: protos.StatusCode_OK}, nil
}

func (s *Service) DeleteBucket(_ context.Context, bucket *protos.Bucket) (*protos.StatusResponse, error) {
	logger.InfoLogger.Println("delete bucket", bucket)
	if err := s.processor.DeleteBucket(bucket); err != nil {
		return &protos.StatusResponse{Code: protos.StatusCode_NOT_FOUND}, nil
	}
	return &protos.StatusResponse{Code: protos.StatusCode_OK}, nil
}

func (s *Service) ListBuckets(_ context.Context, _ *protos.EmptyParams) (*protos.BucketListResponse, error) {
	logger.InfoLogger.Println("list buckets")
	return s.processor.ListBuckets()
}

func (s *Service) LookupBucket(_ context.Context, bucket *protos.Bucket) (*protos.StatusResponse, error) {
	logger.InfoLogger.Println("look up bucket", bucket)
	if err := s.processor.LookupBucket(bucket); err != nil {
		return &protos.StatusResponse{Code: protos.StatusCode_NOT_FOUND}, nil
	}
	return &protos.StatusResponse{Code: protos.StatusCode_OK}, nil
}

func (s *Service) Create(_ context.Context, object *protos.Object) (*protos.StatusResponse, error) {
	logger.InfoLogger.Println("create object", object)
	s.metrics.IncrementCreateObject()
	if err := s.processor.CreateObject(object); err != nil {
		return &protos.StatusResponse{Code: protos.StatusCode_ALREADY_EXISTS}, nil
	}
	return &protos.StatusResponse{Code: protos.StatusCode_OK}, nil
}

func (s *Service) Update(_ context.Context, object *protos.Object) (*protos.StatusResponse, error) {
	logger.InfoLogger.Println("update object", object)
	s.metrics.IncrementUpdateObject()
	if err := s.processor.UpdateObject(object); err != nil {
		return &protos.StatusResponse{Code: protos.StatusCode_INTERNAL}, nil
	}
	return &protos.StatusResponse{Code: protos.StatusCode_OK}, nil
}

func (s *Service) Delete(_ context.Context, objectID *protos.ObjectID) (*protos.StatusResponse, error) {
	logger.InfoLogger.Println("delete object", objectID)
	s.metrics.IncrementDeleteObject()
	if err := s.processor.DeleteObject(objectID); err != nil {
		return &protos.StatusResponse{Code: protos.StatusCode_NOT_FOUND}, nil
	}
	return &protos.StatusResponse{Code: protos.StatusCode_OK}, nil
}

func (s *Service) DeletePrefix(_ context.Context, objectID *protos.ObjectID) (*protos.StatusResponse, error) {
	logger.InfoLogger.Println("delete object", objectID)
	s.metrics.IncrementDeleteObject()
	if err := s.processor.DeletePrefix(objectID); err != nil {
		return &protos.StatusResponse{Code: protos.StatusCode_NOT_FOUND}, nil
	}
	return &protos.StatusResponse{Code: protos.StatusCode_OK}, nil
}

func (s *Service) Lookup(_ context.Context, objectID *protos.ObjectID) (*protos.ObjectResponse, error) {
	logger.InfoLogger.Println("lookup object", objectID)
	s.metrics.IncrementLookupObject()
	object, err := s.processor.LookupObject(objectID)
	if err != nil {
		return &protos.ObjectResponse{
			Error: &protos.StatusResponse{Code: protos.StatusCode_NOT_FOUND},
		}, err
	}
	return object, nil
}

func (s *Service) List(_ context.Context, objectListRequest *protos.ObjectListRequest) (*protos.ObjectListResponse, error) {
	logger.InfoLogger.Println("list objects")
	s.metrics.IncrementListObject()
	return s.processor.List(objectListRequest)
}

func (s *Service) Subscribe(_ context.Context, subscription *protos.SubscriptionEvent) (*protos.StatusResponse, error) {
	if err := s.processor.Subscribe(subscription); err != nil {
		return &protos.StatusResponse{Code: protos.StatusCode_INTERNAL}, err
	}
	return &protos.StatusResponse{Code: protos.StatusCode_OK}, nil
}

func (s *Service) SubscribeStream(subscription *protos.SubscriptionStreamEvent,
	stream protos.MetadataService_SubscribeStreamServer) error {
	return s.processor.SubscribeStream(subscription, stream)
}

func (s *Service) Unsubscribe(_ context.Context, unsubscribe *protos.SubscriptionEvent) (*protos.StatusResponse, error) {
	if err := s.processor.Unsubscribe(unsubscribe); err != nil {
		return &protos.StatusResponse{Code: protos.StatusCode_NOT_FOUND}, nil
	}
	return &protos.StatusResponse{Code: protos.StatusCode_OK}, nil
}
