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
	"log"
	"net"
	"os"
)

func mockServer(ctx context.Context) (protos.MetadataServiceClient, func()) {
	metrics := &prommetrics.Metrics{}
	_ = os.RemoveAll("./data")
	buffer := 101024 * 1024
	lis := bufconn.Listen(buffer)
	maxMessageSize := 64 * 1024 * 1024
	opts := []grpc.ServerOption{grpc.KeepaliveEnforcementPolicy(serverconfig.KAEP),
		grpc.KeepaliveParams(serverconfig.KASP),
		grpc.MaxRecvMsgSize(maxMessageSize), grpc.MaxSendMsgSize(maxMessageSize)}
	grpcServer := grpc.NewServer(opts...)
	serviceInstance := mdsservice.NewService(metrics)
	protos.RegisterMetadataServiceServer(grpcServer, serviceInstance)
	go func() {
		if err := grpcServer.Serve(lis); err != nil {
			logger.ErrorLogger.Println("error serving server: ", err)
		}
	}()
	conn, err := grpc.DialContext(ctx, "",
		grpc.WithContextDialer(func(context.Context, string) (net.Conn, error) {
			return lis.Dial()
		}), grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		logger.ErrorLogger.Println("error connecting to server: ", err)
	}
	closer := func() {
		err = lis.Close()
		if err != nil {
			log.Printf("error closing listener: %v", err)
		}
		grpcServer.Stop()
	}
	client := protos.NewMetadataServiceClient(conn)
	return client, closer
}
