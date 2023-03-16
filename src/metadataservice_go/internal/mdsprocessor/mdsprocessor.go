package mdsprocessor

import (
	"github.com/IBM/gedsmds/internal/config"
	"github.com/IBM/gedsmds/internal/connection/connpool"
	"github.com/IBM/gedsmds/internal/keyvaluestore"
	"github.com/IBM/gedsmds/internal/logger"
	"github.com/IBM/gedsmds/internal/pubsub"
	"github.com/IBM/gedsmds/protos/protos"
)

func InitService() *Service {
	kvStore := keyvaluestore.InitKeyValueStoreService()
	return &Service{
		pubsub:  pubsub.InitService(kvStore),
		kvStore: kvStore,
	}
}

func (s *Service) GetConnectionInformation() string {
	return connpool.GetOutboundIP()
}

func (s *Service) RegisterObjectStore(objectStore *protos.ObjectStoreConfig) error {
	if err := s.kvStore.RegisterObjectStore(objectStore); err != nil {
		return err
	}
	return nil
}

func (s *Service) ListObjectStores() (*protos.AvailableObjectStoreConfigs, error) {
	return s.kvStore.ListObjectStores()
}

func (s *Service) CreateBucket(bucket *protos.Bucket) error {
	if err := s.kvStore.CreateBucket(bucket); err != nil {
		return err
	}
	return nil
}

func (s *Service) DeleteBucket(bucket *protos.Bucket) error {
	if err := s.kvStore.DeleteBucket(bucket); err != nil {
		return err
	}
	return nil
}

func (s *Service) ListBuckets() (*protos.BucketListResponse, error) {
	return s.kvStore.ListBuckets()
}

func (s *Service) LookupBucket(bucket *protos.Bucket) error {
	if err := s.kvStore.LookupBucket(bucket); err != nil {
		return err
	}
	return nil
}

func (s *Service) CreateObject(object *protos.Object) error {
	if err := s.kvStore.CreateObject(object); err != nil {
		return err
	}
	if config.Config.PubSubEnabled {
		s.pubsub.Publication <- object
	}
	return nil
}

func (s *Service) CreateOrUpdateObjectStream(object *protos.Object) {
	if err := s.kvStore.CreateObject(object); err != nil {
		logger.ErrorLogger.Println(err)
		return
	}
	if config.Config.PubSubEnabled {
		s.pubsub.Publication <- object
	}
}

func (s *Service) UpdateObject(object *protos.Object) error {
	if err := s.kvStore.UpdateObject(object); err != nil {
		return err
	}
	if config.Config.PubSubEnabled {
		s.pubsub.Publication <- object
	}
	return nil
}

func (s *Service) DeleteObject(objectID *protos.ObjectID) error {
	if err := s.kvStore.DeleteObject(objectID); err != nil {
		return err
	}
	if config.Config.PubSubEnabled {
		s.pubsub.Publication <- &protos.Object{
			Id: objectID,
		}
	}
	return nil
}

func (s *Service) DeletePrefix(objectID *protos.ObjectID) error {
	objects, err := s.kvStore.DeleteObjectPrefix(objectID)
	if err != nil {
		return err
	}
	if config.Config.PubSubEnabled {
		for _, object := range objects {
			s.pubsub.Publication <- object
		}
	}
	return nil
}

func (s *Service) LookupObject(objectID *protos.ObjectID) (*protos.ObjectResponse, error) {
	object, err := s.kvStore.LookupObject(objectID)
	if err != nil {
		return &protos.ObjectResponse{
			Error: &protos.StatusResponse{Code: protos.StatusCode_NOT_FOUND},
		}, nil
	}
	object.Error = &protos.StatusResponse{Code: protos.StatusCode_OK}
	return object, nil
}

func (s *Service) List(objectListRequest *protos.ObjectListRequest) (*protos.ObjectListResponse, error) {
	return s.kvStore.ListObjects(objectListRequest)
}

func (s *Service) Subscribe(subscription *protos.SubscriptionEvent) error {
	return s.pubsub.Subscribe(subscription)
}

func (s *Service) SubscribeStream(subscription *protos.SubscriptionStreamEvent,
	stream protos.MetadataService_SubscribeStreamServer) error {
	return s.pubsub.SubscribeStream(subscription, stream)
}

func (s *Service) Unsubscribe(unsubscribe *protos.SubscriptionEvent) error {
	if err := s.pubsub.Unsubscribe(unsubscribe); err != nil {
		return err
	}
	return nil
}
