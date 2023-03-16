package main

import (
	"github.com/IBM/gedsmds/internal/config"
	"github.com/IBM/gedsmds/internal/connection/serverconfig"
	"github.com/IBM/gedsmds/internal/logger"
	"github.com/IBM/gedsmds/internal/mdsservice"
	"github.com/IBM/gedsmds/protos/protos"
	"google.golang.org/grpc"
	//_ "google.golang.org/grpc/encoding/gzip"
	"net"
)

func main() {
	lis, err := net.Listen("tcp", config.Config.MDSPort)
	if err != nil {
		logger.FatalLogger.Fatalln(err)
	}
	opts := []grpc.ServerOption{grpc.KeepaliveEnforcementPolicy(serverconfig.KAEP),
		grpc.KeepaliveParams(serverconfig.KASP)}
	grpcServer := grpc.NewServer(opts...)
	serviceInstance := mdsservice.NewService()
	protos.RegisterMetadataServiceServer(grpcServer, serviceInstance)
	logger.InfoLogger.Println("Metadata Server is listening on port", config.Config.MDSPort)
	//logger.InfoLogger.Println("My IP:", serviceInstance.GetIP())
	err = grpcServer.Serve(lis)
	if err != nil {
		logger.FatalLogger.Fatalln(err)
	}
}
