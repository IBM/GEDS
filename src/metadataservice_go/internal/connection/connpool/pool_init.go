package connpool

import (
	"github.com/IBM/gedsmds/internal/config"
	"github.com/IBM/gedsmds/internal/logger"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
	"google.golang.org/grpc/keepalive"
	"time"
)

var KACP = keepalive.ClientParameters{
	Time:                10 * time.Second, // send pings every 10 seconds if there is no activity
	Timeout:             10 * time.Second, // wait 30 second for ping ack before considering the connection dead
	PermitWithoutStream: true,             // send pings even without active streams
}

func factoryNode(ip string) (*grpc.ClientConn, error) {
	opts := []grpc.DialOption{grpc.WithTransportCredentials(insecure.NewCredentials()), grpc.WithKeepaliveParams(KACP)}
	conn, err := grpc.Dial(ip+config.Config.MDSPort, opts...)
	if err != nil {
		logger.FatalLogger.Fatalln("Failed to start gRPC connection:", err)
	}
	return conn, err
}

func GetMDSConnectionsStream() map[string]*Pool {
	serverPool := make(map[string]*Pool)
	name := "127.0.0.1"
	//connection := "172.17.0.2"
	pool, err := NewPoolWithIP(factoryNode, name, 10, 10, 10*time.Second)
	if err != nil {
		logger.FatalLogger.Fatalln("Failed to create gRPC pool:", err)
	}
	serverPool[name] = pool
	return serverPool
}

func SleepAndContinue() {
	time.Sleep(2 * time.Second)
}
