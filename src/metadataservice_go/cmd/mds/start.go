package main

import (
	"github.com/IBM/gedsmds/internal/config"
	"github.com/IBM/gedsmds/internal/connection/serverconfig"
	"github.com/IBM/gedsmds/internal/logger"
	"github.com/IBM/gedsmds/internal/mdsservice"
	"github.com/IBM/gedsmds/internal/prommetrics"
	"github.com/IBM/gedsmds/protos"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/collectors"
	"github.com/prometheus/client_golang/prometheus/promhttp"
	"google.golang.org/grpc"
	"net/http"
	//_ "google.golang.org/grpc/encoding/gzip"
	"net"
)

func main() {
	go prometheusServer()
	mdsServer()
}

func mdsServer() {
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
	err = grpcServer.Serve(lis)
	if err != nil {
		logger.FatalLogger.Fatalln(err)
	}
}

func prometheusServer() {
	logger.InfoLogger.Println("Prometheus endpoint is listening on port", config.Config.PrometheusPort)
	registry := prometheus.NewRegistry()
	registry.MustRegister(collectors.NewGoCollector())
	prommetrics.InitMetrics(registry)
	handler := promhttp.HandlerFor(registry, promhttp.HandlerOpts{Registry: registry})
	http.Handle("/metrics", handler)
	err := http.ListenAndServe(config.Config.PrometheusPort, nil)
	if err != nil {
		logger.FatalLogger.Fatalln(err)
	}
}
