package serverconfig

import (
	"google.golang.org/grpc/keepalive"
	"time"
)

var KAEP = keepalive.EnforcementPolicy{
	MinTime:             5 * time.Second, // If a client pings more than once every 5 seconds, terminate the connection
	PermitWithoutStream: true,            // Allow pings even when there are no active streams
}

var KASP = keepalive.ServerParameters{
	MaxConnectionIdle:     10 * time.Minute, // If a client is idle for 60 seconds, send a GOAWAY
	MaxConnectionAge:      10 * time.Minute, // If any connection is alive for more than 10 minutes, send a GOAWAY
	MaxConnectionAgeGrace: 30 * time.Second, // Allow 30 seconds for pending RPCs to complete before forcibly closing connections
	Time:                  5 * time.Second,  // Ping the client if it is idle for 5 seconds to ensure the connection is still active
	Timeout:               30 * time.Second, // Wait 30 second for the ping ack before assuming the connection is dead
}
