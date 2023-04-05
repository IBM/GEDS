package pubsub

import (
	"github.com/IBM/gedsmds/internal/keyvaluestore"
	"github.com/IBM/gedsmds/internal/logger"
	"github.com/IBM/gedsmds/protos"
	"strings"
	"sync"
)

func InitService(kvStore *keyvaluestore.Service) *Service {
	service := &Service{
		kvStore: kvStore,

		subscribersStreamLock: &sync.RWMutex{},
		subscriberStreams:     map[string]*SubscriberStream{},

		subscribedItemsLock: &sync.RWMutex{},
		subscribedItems:     map[string][]string{},

		subscribedPrefixLock: &sync.RWMutex{},
		subscribedPrefix:     map[string]map[string][]string{},

		Publication: make(chan *protos.SubscriptionStreamResponse, channelBufferSize),
	}
	go service.runPubSubEventListeners()
	return service
}

func (s *Service) runPubSubEventListeners() {
	for {
		go s.matchPubSub(<-s.Publication)
	}
}

func (s *Service) Subscribe(subscription *protos.SubscriptionEvent) error {
	subscribedItemId, err := s.createSubscriptionKey(subscription)
	if err != nil {
		return err
	}
	if subscription.SubscriptionType == protos.SubscriptionType_BUCKET || subscription.SubscriptionType == protos.SubscriptionType_OBJECT {
		s.subscribedItemsLock.Lock()
		s.subscribedItems[subscribedItemId] = append(s.subscribedItems[subscribedItemId], subscription.SubscriberID)
		s.subscribedItemsLock.Unlock()
	} else if subscription.SubscriptionType == protos.SubscriptionType_PREFIX {
		s.subscribedPrefixLock.Lock()
		if _, ok := s.subscribedPrefix[subscription.BucketID]; !ok {
			s.subscribedPrefix[subscription.BucketID] = map[string][]string{}
		}
		s.subscribedPrefix[subscription.BucketID][subscribedItemId] = append(s.subscribedPrefix[subscription.BucketID][subscribedItemId], subscription.SubscriberID)
		s.subscribedPrefixLock.Unlock()
	}
	s.subscribersStreamLock.Lock()
	defer s.subscribersStreamLock.Unlock()
	if streamer, ok := s.subscriberStreams[subscription.SubscriberID]; ok {
		streamer.subscriptionsCounter++
	} else {
		s.subscriberStreams[subscription.SubscriberID] = &SubscriberStream{
			subscriptionsCounter: 1,
		}
	}
	return nil
}

func (s *Service) SubscribeStream(subscription *protos.SubscriptionStreamEvent,
	stream protos.MetadataService_SubscribeStreamServer) error {
	logger.ErrorLogger.Println("got subscription: ", subscription)

	finished := make(chan bool, 2)
	s.subscribersStreamLock.Lock()
	if streamer, ok := s.subscriberStreams[subscription.SubscriberID]; !ok {
		s.subscriberStreams[subscription.SubscriberID] = &SubscriberStream{
			stream:   stream,
			finished: finished,
		}
	} else {
		s.removeSubscriberStream(streamer)
		streamer.stream = stream
		streamer.finished = finished
	}
	s.subscribersStreamLock.Unlock()
	cntx := stream.Context()
	for {
		select {
		case <-finished:
			return nil
		case <-cntx.Done():
			return nil
		}
	}
}

func (s *Service) matchPubSub(publication *protos.SubscriptionStreamResponse) {
	var subscribers []string
	bucketID, objectID := s.createSubscriptionKeyForMatching(publication.Object)
	s.subscribedItemsLock.RLock()
	if currentSubscribers, ok := s.subscribedItems[bucketID]; ok {
		subscribers = append(subscribers, currentSubscribers...)
	}
	if currentSubscribers, ok := s.subscribedItems[objectID]; ok {
		subscribers = append(subscribers, currentSubscribers...)
	}
	s.subscribedItemsLock.RUnlock()
	s.subscribedPrefixLock.RLock()
	if subscribersInBucket, ok := s.subscribedPrefix[publication.Object.Id.Bucket]; ok {
		for prefix, currentSubscribers := range subscribersInBucket {
			if strings.HasPrefix(objectID, prefix) {
				subscribers = append(subscribers, currentSubscribers...)
			}
		}
	}
	s.subscribedPrefixLock.RUnlock()

	sentSubscriptions := map[string]bool{}
	for _, subscriberID := range subscribers {
		if _, ok := sentSubscriptions[subscriberID]; !ok {
			s.sendPublication(publication, subscriberID)
			sentSubscriptions[subscriberID] = true
		}
	}
}

func (s *Service) sendPublication(publication *protos.SubscriptionStreamResponse, subscriberID string) {
	s.subscribersStreamLock.RLock()
	streamer, ok := s.subscriberStreams[subscriberID]
	s.subscribersStreamLock.RUnlock()

	if !ok || streamer.stream == nil {
		logger.ErrorLogger.Println("subscriber stream not found: " + subscriberID)
		return
	}
	if err := streamer.stream.Send(publication); err != nil {
		logger.ErrorLogger.Println("could not send the publication to subscriber " + subscriberID)
		s.removeSubscriberStreamWithLock(streamer)
	}
}

func (s *Service) removeSubscriberStream(streamer *SubscriberStream) {
	if streamer.stream != nil {
		streamer.finished <- true
		streamer.stream = nil
	}
}

func (s *Service) removeSubscriberStreamWithLock(streamer *SubscriberStream) {
	s.subscribersStreamLock.Lock()
	if streamer.stream != nil {
		streamer.finished <- true
		streamer.stream = nil
	}
	s.subscribersStreamLock.Unlock()
}

func (s *Service) removeSubscriber(unsubscription *protos.SubscriptionEvent) error {
	s.subscribersStreamLock.Lock()
	if streamer, ok := s.subscriberStreams[unsubscription.SubscriberID]; ok {
		streamer.subscriptionsCounter--
		if streamer.subscriptionsCounter <= 0 {
			s.removeSubscriberStream(streamer)
		}
	}
	s.subscribersStreamLock.Unlock()

	subscribedItemId, err := s.createSubscriptionKey(unsubscription)
	if err != nil {
		return err
	}
	// possible bottleneck
	s.subscribedItemsLock.Lock()
	if currentSubscribers, ok := s.subscribedItems[subscribedItemId]; ok {
		s.removeElementFromSlice(currentSubscribers, unsubscription.SubscriberID)
	}
	s.subscribedItemsLock.Unlock()
	s.subscribedPrefixLock.Lock()
	if subscribers, ok := s.subscribedPrefix[unsubscription.BucketID]; ok {
		s.removeElementFromSlice(subscribers[subscribedItemId], unsubscription.SubscriberID)
	}
	s.subscribedPrefixLock.Unlock()
	return nil
}

func (s *Service) Unsubscribe(unsubscription *protos.SubscriptionEvent) error {
	return s.removeSubscriber(unsubscription)
}
