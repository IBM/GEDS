package mdsservice

import (
	"context"
	"github.com/IBM/gedsmds/internal/mdsprocessor"
	"github.com/IBM/gedsmds/protos/protos"
	"io"
)

func NewService() *Service {
	return &Service{
		processor: mdsprocessor.InitService(),
	}
}

func (s *Service) GetConnectionInformation(_ context.Context,
	_ *protos.EmptyParams) (*protos.ConnectionInformation, error) {
	return &protos.ConnectionInformation{RemoteAddress: s.processor.GetConnectionInformation()}, nil
}

func (s *Service) RegisterObjectStore(_ context.Context,
	objectStore *protos.ObjectStoreConfig) (*protos.StatusResponse, error) {
	if err := s.processor.RegisterObjectStore(objectStore); err != nil {
		return &protos.StatusResponse{Code: protos.StatusCode_ALREADY_EXISTS}, nil
	}
	return &protos.StatusResponse{Code: protos.StatusCode_OK}, nil
}

func (s *Service) ListObjectStores(_ context.Context,
	_ *protos.EmptyParams) (*protos.AvailableObjectStoreConfigs, error) {
	return s.processor.ListObjectStores()
}

func (s *Service) CreateBucket(_ context.Context, bucket *protos.Bucket) (*protos.StatusResponse, error) {
	if err := s.processor.CreateBucket(bucket); err != nil {
		return &protos.StatusResponse{Code: protos.StatusCode_ALREADY_EXISTS}, nil
	}
	return &protos.StatusResponse{Code: protos.StatusCode_OK}, nil
}

func (s *Service) DeleteBucket(_ context.Context, bucket *protos.Bucket) (*protos.StatusResponse, error) {
	if err := s.processor.DeleteBucket(bucket); err != nil {
		return &protos.StatusResponse{Code: protos.StatusCode_NOT_FOUND}, nil
	}
	return &protos.StatusResponse{Code: protos.StatusCode_OK}, nil
}

func (s *Service) ListBuckets(_ context.Context, _ *protos.EmptyParams) (*protos.BucketListResponse, error) {
	return s.processor.ListBuckets()
}

func (s *Service) LookupBucket(_ context.Context, bucket *protos.Bucket) (*protos.StatusResponse, error) {
	if err := s.processor.LookupBucket(bucket); err != nil {
		return &protos.StatusResponse{Code: protos.StatusCode_NOT_FOUND}, nil
	}
	return &protos.StatusResponse{Code: protos.StatusCode_OK}, nil
}

func (s *Service) Create(_ context.Context, object *protos.Object) (*protos.StatusResponse, error) {
	if err := s.processor.CreateObject(object); err != nil {
		return &protos.StatusResponse{Code: protos.StatusCode_ALREADY_EXISTS}, nil
	}
	return &protos.StatusResponse{Code: protos.StatusCode_OK}, nil
}

func (s *Service) Update(_ context.Context, object *protos.Object) (*protos.StatusResponse, error) {
	if err := s.processor.UpdateObject(object); err != nil {
		return &protos.StatusResponse{Code: protos.StatusCode_INTERNAL}, nil
	}
	return &protos.StatusResponse{Code: protos.StatusCode_OK}, nil
}

func (s *Service) Delete(_ context.Context, objectID *protos.ObjectID) (*protos.StatusResponse, error) {
	if err := s.processor.DeleteObject(objectID); err != nil {
		return &protos.StatusResponse{Code: protos.StatusCode_NOT_FOUND}, nil
	}
	return &protos.StatusResponse{Code: protos.StatusCode_OK}, nil
}

func (s *Service) DeletePrefix(_ context.Context, objectID *protos.ObjectID) (*protos.StatusResponse, error) {
	if err := s.processor.DeletePrefix(objectID); err != nil {
		return &protos.StatusResponse{Code: protos.StatusCode_NOT_FOUND}, nil
	}
	return &protos.StatusResponse{Code: protos.StatusCode_OK}, nil
}

func (s *Service) Lookup(_ context.Context, objectID *protos.ObjectID) (*protos.ObjectResponse, error) {
	object, err := s.processor.LookupObject(objectID)
	if err != nil {
		return &protos.ObjectResponse{
			Error: &protos.StatusResponse{Code: protos.StatusCode_NOT_FOUND},
		}, err
	}
	object.Error = &protos.StatusResponse{Code: protos.StatusCode_OK}
	return object, nil
}

func (s *Service) List(_ context.Context, objectListRequest *protos.ObjectListRequest) (*protos.ObjectListResponse, error) {
	return s.processor.List(objectListRequest)
}

func (s *Service) CreateOrUpdateObjectStream(stream protos.MetadataService_CreateOrUpdateObjectStreamServer) error {
	for {
		object, err := stream.Recv()
		if err == io.EOF {
			return stream.SendAndClose(&protos.StatusResponse{Code: protos.StatusCode_OK})
		}
		if err != nil {
			return err
		}
		s.processor.CreateOrUpdateObjectStream(object)
	}
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
