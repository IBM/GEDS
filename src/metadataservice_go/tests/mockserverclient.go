/**
* Copyright 2023- Pezhman Nasirifard. All rights reserved
* SPDX-License-Identifier: Apache-2.0
 */

package main

import (
	"context"
	"github.com/IBM/gedsmds/internal/connection/serverconfig"
	"github.com/IBM/gedsmds/internal/logger"
	"github.com/IBM/gedsmds/internal/mdsservice"
	"github.com/IBM/gedsmds/internal/prommetrics"
	"github.com/IBM/gedsmds/protos"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
	"google.golang.org/grpc/test/bufconn"
	"net"
	"os"
)

func mockServerClient(ctx context.Context) (protos.MetadataServiceClient, func()) {
	metrics := &prommetrics.Metrics{}
	_ = os.RemoveAll("./data")
	buffer := 101024 * 1024
	listener := bufconn.Listen(buffer)
	maxMessageSize := 64 * 1024 * 1024
	opts := []grpc.ServerOption{grpc.KeepaliveEnforcementPolicy(serverconfig.KAEP),
		grpc.KeepaliveParams(serverconfig.KASP),
		grpc.MaxRecvMsgSize(maxMessageSize), grpc.MaxSendMsgSize(maxMessageSize)}
	mds := grpc.NewServer(opts...)
	service := mdsservice.NewService(metrics)
	protos.RegisterMetadataServiceServer(mds, service)
	go func() {
		if err := mds.Serve(listener); err != nil {
			logger.ErrorLogger.Println("error serving server: ", err)
		}
	}()
	conn, err := grpc.DialContext(ctx, "",
		grpc.WithContextDialer(func(context.Context, string) (net.Conn, error) {
			return listener.Dial()
		}), grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		logger.ErrorLogger.Println("error connecting to server: ", err)
	}
	closer := func() {
		if err = listener.Close(); err != nil {
			logger.ErrorLogger.Println("error closing listener: ", err)
		}
		mds.Stop()
	}
	client := protos.NewMetadataServiceClient(conn)
	return client, closer
}
