//
// Copyright 2022- IBM Inc. All rights reserved
// SPDX-License-Identifier: Apache-2.0
//

// Code generated by protoc-gen-go-grpc. DO NOT EDIT.
// versions:
// - protoc-gen-go-grpc v1.3.0
// - protoc             v3.12.4
// source: geds.proto

package protos

import (
	context "context"
	grpc "google.golang.org/grpc"
	codes "google.golang.org/grpc/codes"
	status "google.golang.org/grpc/status"
)

// This is a compile-time assertion to ensure that this generated file
// is compatible with the grpc package it is being compiled against.
// Requires gRPC-Go v1.32.0 or later.
const _ = grpc.SupportPackageIsVersion7

const (
	MetadataService_GetConnectionInformation_FullMethodName = "/geds.rpc.MetadataService/GetConnectionInformation"
	MetadataService_RegisterObjectStore_FullMethodName      = "/geds.rpc.MetadataService/RegisterObjectStore"
	MetadataService_ListObjectStores_FullMethodName         = "/geds.rpc.MetadataService/ListObjectStores"
	MetadataService_CreateBucket_FullMethodName             = "/geds.rpc.MetadataService/CreateBucket"
	MetadataService_DeleteBucket_FullMethodName             = "/geds.rpc.MetadataService/DeleteBucket"
	MetadataService_ListBuckets_FullMethodName              = "/geds.rpc.MetadataService/ListBuckets"
	MetadataService_LookupBucket_FullMethodName             = "/geds.rpc.MetadataService/LookupBucket"
	MetadataService_Create_FullMethodName                   = "/geds.rpc.MetadataService/Create"
	MetadataService_Update_FullMethodName                   = "/geds.rpc.MetadataService/Update"
	MetadataService_Delete_FullMethodName                   = "/geds.rpc.MetadataService/Delete"
	MetadataService_DeletePrefix_FullMethodName             = "/geds.rpc.MetadataService/DeletePrefix"
	MetadataService_Lookup_FullMethodName                   = "/geds.rpc.MetadataService/Lookup"
	MetadataService_List_FullMethodName                     = "/geds.rpc.MetadataService/List"
	MetadataService_Subscribe_FullMethodName                = "/geds.rpc.MetadataService/Subscribe"
	MetadataService_SubscribeStream_FullMethodName          = "/geds.rpc.MetadataService/SubscribeStream"
	MetadataService_Unsubscribe_FullMethodName              = "/geds.rpc.MetadataService/Unsubscribe"
)

// MetadataServiceClient is the client API for MetadataService service.
//
// For semantics around ctx use and closing/ending streaming RPCs, please refer to https://pkg.go.dev/google.golang.org/grpc/?tab=doc#ClientConn.NewStream.
type MetadataServiceClient interface {
	GetConnectionInformation(ctx context.Context, in *EmptyParams, opts ...grpc.CallOption) (*ConnectionInformation, error)
	RegisterObjectStore(ctx context.Context, in *ObjectStoreConfig, opts ...grpc.CallOption) (*StatusResponse, error)
	ListObjectStores(ctx context.Context, in *EmptyParams, opts ...grpc.CallOption) (*AvailableObjectStoreConfigs, error)
	CreateBucket(ctx context.Context, in *Bucket, opts ...grpc.CallOption) (*StatusResponse, error)
	DeleteBucket(ctx context.Context, in *Bucket, opts ...grpc.CallOption) (*StatusResponse, error)
	ListBuckets(ctx context.Context, in *EmptyParams, opts ...grpc.CallOption) (*BucketListResponse, error)
	LookupBucket(ctx context.Context, in *Bucket, opts ...grpc.CallOption) (*StatusResponse, error)
	Create(ctx context.Context, in *Object, opts ...grpc.CallOption) (*StatusResponse, error)
	Update(ctx context.Context, in *Object, opts ...grpc.CallOption) (*StatusResponse, error)
	Delete(ctx context.Context, in *ObjectID, opts ...grpc.CallOption) (*StatusResponse, error)
	DeletePrefix(ctx context.Context, in *ObjectID, opts ...grpc.CallOption) (*StatusResponse, error)
	Lookup(ctx context.Context, in *ObjectID, opts ...grpc.CallOption) (*ObjectResponse, error)
	List(ctx context.Context, in *ObjectListRequest, opts ...grpc.CallOption) (*ObjectListResponse, error)
	Subscribe(ctx context.Context, in *SubscriptionEvent, opts ...grpc.CallOption) (*StatusResponse, error)
	SubscribeStream(ctx context.Context, in *SubscriptionStreamEvent, opts ...grpc.CallOption) (MetadataService_SubscribeStreamClient, error)
	Unsubscribe(ctx context.Context, in *SubscriptionEvent, opts ...grpc.CallOption) (*StatusResponse, error)
}

type metadataServiceClient struct {
	cc grpc.ClientConnInterface
}

func NewMetadataServiceClient(cc grpc.ClientConnInterface) MetadataServiceClient {
	return &metadataServiceClient{cc}
}

func (c *metadataServiceClient) GetConnectionInformation(ctx context.Context, in *EmptyParams, opts ...grpc.CallOption) (*ConnectionInformation, error) {
	out := new(ConnectionInformation)
	err := c.cc.Invoke(ctx, MetadataService_GetConnectionInformation_FullMethodName, in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *metadataServiceClient) RegisterObjectStore(ctx context.Context, in *ObjectStoreConfig, opts ...grpc.CallOption) (*StatusResponse, error) {
	out := new(StatusResponse)
	err := c.cc.Invoke(ctx, MetadataService_RegisterObjectStore_FullMethodName, in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *metadataServiceClient) ListObjectStores(ctx context.Context, in *EmptyParams, opts ...grpc.CallOption) (*AvailableObjectStoreConfigs, error) {
	out := new(AvailableObjectStoreConfigs)
	err := c.cc.Invoke(ctx, MetadataService_ListObjectStores_FullMethodName, in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *metadataServiceClient) CreateBucket(ctx context.Context, in *Bucket, opts ...grpc.CallOption) (*StatusResponse, error) {
	out := new(StatusResponse)
	err := c.cc.Invoke(ctx, MetadataService_CreateBucket_FullMethodName, in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *metadataServiceClient) DeleteBucket(ctx context.Context, in *Bucket, opts ...grpc.CallOption) (*StatusResponse, error) {
	out := new(StatusResponse)
	err := c.cc.Invoke(ctx, MetadataService_DeleteBucket_FullMethodName, in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *metadataServiceClient) ListBuckets(ctx context.Context, in *EmptyParams, opts ...grpc.CallOption) (*BucketListResponse, error) {
	out := new(BucketListResponse)
	err := c.cc.Invoke(ctx, MetadataService_ListBuckets_FullMethodName, in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *metadataServiceClient) LookupBucket(ctx context.Context, in *Bucket, opts ...grpc.CallOption) (*StatusResponse, error) {
	out := new(StatusResponse)
	err := c.cc.Invoke(ctx, MetadataService_LookupBucket_FullMethodName, in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *metadataServiceClient) Create(ctx context.Context, in *Object, opts ...grpc.CallOption) (*StatusResponse, error) {
	out := new(StatusResponse)
	err := c.cc.Invoke(ctx, MetadataService_Create_FullMethodName, in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *metadataServiceClient) Update(ctx context.Context, in *Object, opts ...grpc.CallOption) (*StatusResponse, error) {
	out := new(StatusResponse)
	err := c.cc.Invoke(ctx, MetadataService_Update_FullMethodName, in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *metadataServiceClient) Delete(ctx context.Context, in *ObjectID, opts ...grpc.CallOption) (*StatusResponse, error) {
	out := new(StatusResponse)
	err := c.cc.Invoke(ctx, MetadataService_Delete_FullMethodName, in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *metadataServiceClient) DeletePrefix(ctx context.Context, in *ObjectID, opts ...grpc.CallOption) (*StatusResponse, error) {
	out := new(StatusResponse)
	err := c.cc.Invoke(ctx, MetadataService_DeletePrefix_FullMethodName, in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *metadataServiceClient) Lookup(ctx context.Context, in *ObjectID, opts ...grpc.CallOption) (*ObjectResponse, error) {
	out := new(ObjectResponse)
	err := c.cc.Invoke(ctx, MetadataService_Lookup_FullMethodName, in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *metadataServiceClient) List(ctx context.Context, in *ObjectListRequest, opts ...grpc.CallOption) (*ObjectListResponse, error) {
	out := new(ObjectListResponse)
	err := c.cc.Invoke(ctx, MetadataService_List_FullMethodName, in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *metadataServiceClient) Subscribe(ctx context.Context, in *SubscriptionEvent, opts ...grpc.CallOption) (*StatusResponse, error) {
	out := new(StatusResponse)
	err := c.cc.Invoke(ctx, MetadataService_Subscribe_FullMethodName, in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *metadataServiceClient) SubscribeStream(ctx context.Context, in *SubscriptionStreamEvent, opts ...grpc.CallOption) (MetadataService_SubscribeStreamClient, error) {
	stream, err := c.cc.NewStream(ctx, &MetadataService_ServiceDesc.Streams[0], MetadataService_SubscribeStream_FullMethodName, opts...)
	if err != nil {
		return nil, err
	}
	x := &metadataServiceSubscribeStreamClient{stream}
	if err := x.ClientStream.SendMsg(in); err != nil {
		return nil, err
	}
	if err := x.ClientStream.CloseSend(); err != nil {
		return nil, err
	}
	return x, nil
}

type MetadataService_SubscribeStreamClient interface {
	Recv() (*SubscriptionStreamResponse, error)
	grpc.ClientStream
}

type metadataServiceSubscribeStreamClient struct {
	grpc.ClientStream
}

func (x *metadataServiceSubscribeStreamClient) Recv() (*SubscriptionStreamResponse, error) {
	m := new(SubscriptionStreamResponse)
	if err := x.ClientStream.RecvMsg(m); err != nil {
		return nil, err
	}
	return m, nil
}

func (c *metadataServiceClient) Unsubscribe(ctx context.Context, in *SubscriptionEvent, opts ...grpc.CallOption) (*StatusResponse, error) {
	out := new(StatusResponse)
	err := c.cc.Invoke(ctx, MetadataService_Unsubscribe_FullMethodName, in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

// MetadataServiceServer is the server API for MetadataService service.
// All implementations should embed UnimplementedMetadataServiceServer
// for forward compatibility
type MetadataServiceServer interface {
	GetConnectionInformation(context.Context, *EmptyParams) (*ConnectionInformation, error)
	RegisterObjectStore(context.Context, *ObjectStoreConfig) (*StatusResponse, error)
	ListObjectStores(context.Context, *EmptyParams) (*AvailableObjectStoreConfigs, error)
	CreateBucket(context.Context, *Bucket) (*StatusResponse, error)
	DeleteBucket(context.Context, *Bucket) (*StatusResponse, error)
	ListBuckets(context.Context, *EmptyParams) (*BucketListResponse, error)
	LookupBucket(context.Context, *Bucket) (*StatusResponse, error)
	Create(context.Context, *Object) (*StatusResponse, error)
	Update(context.Context, *Object) (*StatusResponse, error)
	Delete(context.Context, *ObjectID) (*StatusResponse, error)
	DeletePrefix(context.Context, *ObjectID) (*StatusResponse, error)
	Lookup(context.Context, *ObjectID) (*ObjectResponse, error)
	List(context.Context, *ObjectListRequest) (*ObjectListResponse, error)
	Subscribe(context.Context, *SubscriptionEvent) (*StatusResponse, error)
	SubscribeStream(*SubscriptionStreamEvent, MetadataService_SubscribeStreamServer) error
	Unsubscribe(context.Context, *SubscriptionEvent) (*StatusResponse, error)
}

// UnimplementedMetadataServiceServer should be embedded to have forward compatible implementations.
type UnimplementedMetadataServiceServer struct {
}

func (UnimplementedMetadataServiceServer) GetConnectionInformation(context.Context, *EmptyParams) (*ConnectionInformation, error) {
	return nil, status.Errorf(codes.Unimplemented, "method GetConnectionInformation not implemented")
}
func (UnimplementedMetadataServiceServer) RegisterObjectStore(context.Context, *ObjectStoreConfig) (*StatusResponse, error) {
	return nil, status.Errorf(codes.Unimplemented, "method RegisterObjectStore not implemented")
}
func (UnimplementedMetadataServiceServer) ListObjectStores(context.Context, *EmptyParams) (*AvailableObjectStoreConfigs, error) {
	return nil, status.Errorf(codes.Unimplemented, "method ListObjectStores not implemented")
}
func (UnimplementedMetadataServiceServer) CreateBucket(context.Context, *Bucket) (*StatusResponse, error) {
	return nil, status.Errorf(codes.Unimplemented, "method CreateBucket not implemented")
}
func (UnimplementedMetadataServiceServer) DeleteBucket(context.Context, *Bucket) (*StatusResponse, error) {
	return nil, status.Errorf(codes.Unimplemented, "method DeleteBucket not implemented")
}
func (UnimplementedMetadataServiceServer) ListBuckets(context.Context, *EmptyParams) (*BucketListResponse, error) {
	return nil, status.Errorf(codes.Unimplemented, "method ListBuckets not implemented")
}
func (UnimplementedMetadataServiceServer) LookupBucket(context.Context, *Bucket) (*StatusResponse, error) {
	return nil, status.Errorf(codes.Unimplemented, "method LookupBucket not implemented")
}
func (UnimplementedMetadataServiceServer) Create(context.Context, *Object) (*StatusResponse, error) {
	return nil, status.Errorf(codes.Unimplemented, "method Create not implemented")
}
func (UnimplementedMetadataServiceServer) Update(context.Context, *Object) (*StatusResponse, error) {
	return nil, status.Errorf(codes.Unimplemented, "method Update not implemented")
}
func (UnimplementedMetadataServiceServer) Delete(context.Context, *ObjectID) (*StatusResponse, error) {
	return nil, status.Errorf(codes.Unimplemented, "method Delete not implemented")
}
func (UnimplementedMetadataServiceServer) DeletePrefix(context.Context, *ObjectID) (*StatusResponse, error) {
	return nil, status.Errorf(codes.Unimplemented, "method DeletePrefix not implemented")
}
func (UnimplementedMetadataServiceServer) Lookup(context.Context, *ObjectID) (*ObjectResponse, error) {
	return nil, status.Errorf(codes.Unimplemented, "method Lookup not implemented")
}
func (UnimplementedMetadataServiceServer) List(context.Context, *ObjectListRequest) (*ObjectListResponse, error) {
	return nil, status.Errorf(codes.Unimplemented, "method List not implemented")
}
func (UnimplementedMetadataServiceServer) Subscribe(context.Context, *SubscriptionEvent) (*StatusResponse, error) {
	return nil, status.Errorf(codes.Unimplemented, "method Subscribe not implemented")
}
func (UnimplementedMetadataServiceServer) SubscribeStream(*SubscriptionStreamEvent, MetadataService_SubscribeStreamServer) error {
	return status.Errorf(codes.Unimplemented, "method SubscribeStream not implemented")
}
func (UnimplementedMetadataServiceServer) Unsubscribe(context.Context, *SubscriptionEvent) (*StatusResponse, error) {
	return nil, status.Errorf(codes.Unimplemented, "method Unsubscribe not implemented")
}

// UnsafeMetadataServiceServer may be embedded to opt out of forward compatibility for this service.
// Use of this interface is not recommended, as added methods to MetadataServiceServer will
// result in compilation errors.
type UnsafeMetadataServiceServer interface {
	mustEmbedUnimplementedMetadataServiceServer()
}

func RegisterMetadataServiceServer(s grpc.ServiceRegistrar, srv MetadataServiceServer) {
	s.RegisterService(&MetadataService_ServiceDesc, srv)
}

func _MetadataService_GetConnectionInformation_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(EmptyParams)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MetadataServiceServer).GetConnectionInformation(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: MetadataService_GetConnectionInformation_FullMethodName,
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MetadataServiceServer).GetConnectionInformation(ctx, req.(*EmptyParams))
	}
	return interceptor(ctx, in, info, handler)
}

func _MetadataService_RegisterObjectStore_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(ObjectStoreConfig)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MetadataServiceServer).RegisterObjectStore(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: MetadataService_RegisterObjectStore_FullMethodName,
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MetadataServiceServer).RegisterObjectStore(ctx, req.(*ObjectStoreConfig))
	}
	return interceptor(ctx, in, info, handler)
}

func _MetadataService_ListObjectStores_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(EmptyParams)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MetadataServiceServer).ListObjectStores(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: MetadataService_ListObjectStores_FullMethodName,
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MetadataServiceServer).ListObjectStores(ctx, req.(*EmptyParams))
	}
	return interceptor(ctx, in, info, handler)
}

func _MetadataService_CreateBucket_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(Bucket)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MetadataServiceServer).CreateBucket(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: MetadataService_CreateBucket_FullMethodName,
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MetadataServiceServer).CreateBucket(ctx, req.(*Bucket))
	}
	return interceptor(ctx, in, info, handler)
}

func _MetadataService_DeleteBucket_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(Bucket)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MetadataServiceServer).DeleteBucket(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: MetadataService_DeleteBucket_FullMethodName,
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MetadataServiceServer).DeleteBucket(ctx, req.(*Bucket))
	}
	return interceptor(ctx, in, info, handler)
}

func _MetadataService_ListBuckets_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(EmptyParams)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MetadataServiceServer).ListBuckets(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: MetadataService_ListBuckets_FullMethodName,
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MetadataServiceServer).ListBuckets(ctx, req.(*EmptyParams))
	}
	return interceptor(ctx, in, info, handler)
}

func _MetadataService_LookupBucket_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(Bucket)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MetadataServiceServer).LookupBucket(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: MetadataService_LookupBucket_FullMethodName,
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MetadataServiceServer).LookupBucket(ctx, req.(*Bucket))
	}
	return interceptor(ctx, in, info, handler)
}

func _MetadataService_Create_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(Object)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MetadataServiceServer).Create(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: MetadataService_Create_FullMethodName,
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MetadataServiceServer).Create(ctx, req.(*Object))
	}
	return interceptor(ctx, in, info, handler)
}

func _MetadataService_Update_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(Object)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MetadataServiceServer).Update(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: MetadataService_Update_FullMethodName,
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MetadataServiceServer).Update(ctx, req.(*Object))
	}
	return interceptor(ctx, in, info, handler)
}

func _MetadataService_Delete_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(ObjectID)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MetadataServiceServer).Delete(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: MetadataService_Delete_FullMethodName,
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MetadataServiceServer).Delete(ctx, req.(*ObjectID))
	}
	return interceptor(ctx, in, info, handler)
}

func _MetadataService_DeletePrefix_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(ObjectID)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MetadataServiceServer).DeletePrefix(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: MetadataService_DeletePrefix_FullMethodName,
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MetadataServiceServer).DeletePrefix(ctx, req.(*ObjectID))
	}
	return interceptor(ctx, in, info, handler)
}

func _MetadataService_Lookup_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(ObjectID)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MetadataServiceServer).Lookup(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: MetadataService_Lookup_FullMethodName,
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MetadataServiceServer).Lookup(ctx, req.(*ObjectID))
	}
	return interceptor(ctx, in, info, handler)
}

func _MetadataService_List_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(ObjectListRequest)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MetadataServiceServer).List(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: MetadataService_List_FullMethodName,
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MetadataServiceServer).List(ctx, req.(*ObjectListRequest))
	}
	return interceptor(ctx, in, info, handler)
}

func _MetadataService_Subscribe_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(SubscriptionEvent)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MetadataServiceServer).Subscribe(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: MetadataService_Subscribe_FullMethodName,
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MetadataServiceServer).Subscribe(ctx, req.(*SubscriptionEvent))
	}
	return interceptor(ctx, in, info, handler)
}

func _MetadataService_SubscribeStream_Handler(srv interface{}, stream grpc.ServerStream) error {
	m := new(SubscriptionStreamEvent)
	if err := stream.RecvMsg(m); err != nil {
		return err
	}
	return srv.(MetadataServiceServer).SubscribeStream(m, &metadataServiceSubscribeStreamServer{stream})
}

type MetadataService_SubscribeStreamServer interface {
	Send(*SubscriptionStreamResponse) error
	grpc.ServerStream
}

type metadataServiceSubscribeStreamServer struct {
	grpc.ServerStream
}

func (x *metadataServiceSubscribeStreamServer) Send(m *SubscriptionStreamResponse) error {
	return x.ServerStream.SendMsg(m)
}

func _MetadataService_Unsubscribe_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(SubscriptionEvent)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(MetadataServiceServer).Unsubscribe(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: MetadataService_Unsubscribe_FullMethodName,
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(MetadataServiceServer).Unsubscribe(ctx, req.(*SubscriptionEvent))
	}
	return interceptor(ctx, in, info, handler)
}

// MetadataService_ServiceDesc is the grpc.ServiceDesc for MetadataService service.
// It's only intended for direct use with grpc.RegisterService,
// and not to be introspected or modified (even as a copy)
var MetadataService_ServiceDesc = grpc.ServiceDesc{
	ServiceName: "geds.rpc.MetadataService",
	HandlerType: (*MetadataServiceServer)(nil),
	Methods: []grpc.MethodDesc{
		{
			MethodName: "GetConnectionInformation",
			Handler:    _MetadataService_GetConnectionInformation_Handler,
		},
		{
			MethodName: "RegisterObjectStore",
			Handler:    _MetadataService_RegisterObjectStore_Handler,
		},
		{
			MethodName: "ListObjectStores",
			Handler:    _MetadataService_ListObjectStores_Handler,
		},
		{
			MethodName: "CreateBucket",
			Handler:    _MetadataService_CreateBucket_Handler,
		},
		{
			MethodName: "DeleteBucket",
			Handler:    _MetadataService_DeleteBucket_Handler,
		},
		{
			MethodName: "ListBuckets",
			Handler:    _MetadataService_ListBuckets_Handler,
		},
		{
			MethodName: "LookupBucket",
			Handler:    _MetadataService_LookupBucket_Handler,
		},
		{
			MethodName: "Create",
			Handler:    _MetadataService_Create_Handler,
		},
		{
			MethodName: "Update",
			Handler:    _MetadataService_Update_Handler,
		},
		{
			MethodName: "Delete",
			Handler:    _MetadataService_Delete_Handler,
		},
		{
			MethodName: "DeletePrefix",
			Handler:    _MetadataService_DeletePrefix_Handler,
		},
		{
			MethodName: "Lookup",
			Handler:    _MetadataService_Lookup_Handler,
		},
		{
			MethodName: "List",
			Handler:    _MetadataService_List_Handler,
		},
		{
			MethodName: "Subscribe",
			Handler:    _MetadataService_Subscribe_Handler,
		},
		{
			MethodName: "Unsubscribe",
			Handler:    _MetadataService_Unsubscribe_Handler,
		},
	},
	Streams: []grpc.StreamDesc{
		{
			StreamName:    "SubscribeStream",
			Handler:       _MetadataService_SubscribeStream_Handler,
			ServerStreams: true,
		},
	},
	Metadata: "geds.proto",
}

const (
	GEDSService_GetAvailEndpoints_FullMethodName = "/geds.rpc.GEDSService/GetAvailEndpoints"
)

// GEDSServiceClient is the client API for GEDSService service.
//
// For semantics around ctx use and closing/ending streaming RPCs, please refer to https://pkg.go.dev/google.golang.org/grpc/?tab=doc#ClientConn.NewStream.
type GEDSServiceClient interface {
	GetAvailEndpoints(ctx context.Context, in *EmptyParams, opts ...grpc.CallOption) (*AvailTransportEndpoints, error)
}

type gEDSServiceClient struct {
	cc grpc.ClientConnInterface
}

func NewGEDSServiceClient(cc grpc.ClientConnInterface) GEDSServiceClient {
	return &gEDSServiceClient{cc}
}

func (c *gEDSServiceClient) GetAvailEndpoints(ctx context.Context, in *EmptyParams, opts ...grpc.CallOption) (*AvailTransportEndpoints, error) {
	out := new(AvailTransportEndpoints)
	err := c.cc.Invoke(ctx, GEDSService_GetAvailEndpoints_FullMethodName, in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

// GEDSServiceServer is the server API for GEDSService service.
// All implementations should embed UnimplementedGEDSServiceServer
// for forward compatibility
type GEDSServiceServer interface {
	GetAvailEndpoints(context.Context, *EmptyParams) (*AvailTransportEndpoints, error)
}

// UnimplementedGEDSServiceServer should be embedded to have forward compatible implementations.
type UnimplementedGEDSServiceServer struct {
}

func (UnimplementedGEDSServiceServer) GetAvailEndpoints(context.Context, *EmptyParams) (*AvailTransportEndpoints, error) {
	return nil, status.Errorf(codes.Unimplemented, "method GetAvailEndpoints not implemented")
}

// UnsafeGEDSServiceServer may be embedded to opt out of forward compatibility for this service.
// Use of this interface is not recommended, as added methods to GEDSServiceServer will
// result in compilation errors.
type UnsafeGEDSServiceServer interface {
	mustEmbedUnimplementedGEDSServiceServer()
}

func RegisterGEDSServiceServer(s grpc.ServiceRegistrar, srv GEDSServiceServer) {
	s.RegisterService(&GEDSService_ServiceDesc, srv)
}

func _GEDSService_GetAvailEndpoints_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(EmptyParams)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(GEDSServiceServer).GetAvailEndpoints(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: GEDSService_GetAvailEndpoints_FullMethodName,
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(GEDSServiceServer).GetAvailEndpoints(ctx, req.(*EmptyParams))
	}
	return interceptor(ctx, in, info, handler)
}

// GEDSService_ServiceDesc is the grpc.ServiceDesc for GEDSService service.
// It's only intended for direct use with grpc.RegisterService,
// and not to be introspected or modified (even as a copy)
var GEDSService_ServiceDesc = grpc.ServiceDesc{
	ServiceName: "geds.rpc.GEDSService",
	HandlerType: (*GEDSServiceServer)(nil),
	Methods: []grpc.MethodDesc{
		{
			MethodName: "GetAvailEndpoints",
			Handler:    _GEDSService_GetAvailEndpoints_Handler,
		},
	},
	Streams:  []grpc.StreamDesc{},
	Metadata: "geds.proto",
}
