package pubsub

import (
	"github.com/IBM/gedsmds/internal/keyvaluestore"
	"github.com/IBM/gedsmds/protos/protos"
	"sync"
)

const channelBufferSize = 200

type SubscriberStream struct {
	stream               protos.MetadataService_SubscribeStreamServer
	finished             chan bool
	subscriptionsCounter int
}

type Service struct {
	kvStore *keyvaluestore.Service

	subscribersStreamLock *sync.RWMutex
	subscriberStreams     map[string]*SubscriberStream

	subscribedItemsLock *sync.RWMutex
	subscribedItems     map[string][]string

	subscribedPrefixLock *sync.RWMutex
	subscribedPrefix     map[string]map[string][]string

	Publication chan *protos.Object
}
