package mdsprocessor

import (
	"github.com/IBM/gedsmds/internal/keyvaluestore"
	"github.com/IBM/gedsmds/internal/pubsub"
)

type Service struct {
	pubsub  *pubsub.Service
	kvStore *keyvaluestore.Service
}
