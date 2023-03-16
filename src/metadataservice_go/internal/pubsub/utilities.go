package pubsub

import (
	"errors"
	"github.com/IBM/gedsmds/protos/protos"
)

const commonDelimiter = "/"

func (s *Service) createSubscriptionKey(subscription *protos.SubscriptionEvent) (string, error) {
	var id string
	if subscription.SubscriptionType == protos.SubscriptionType_BUCKET {
		id = subscription.BucketID
	} else if subscription.SubscriptionType == protos.SubscriptionType_OBJECT {
		id = subscription.BucketID + "-" + subscription.Key
	} else if subscription.SubscriptionType == protos.SubscriptionType_PREFIX {
		id = subscription.BucketID + "-" + subscription.Key
	} else {
		return "", errors.New("subscription type not found")
	}
	return id, nil
}

func (s *Service) createSubscriptionKeyForMatching(object *protos.Object) (string, string) {
	return object.Id.Bucket, object.Id.Bucket + "-" + object.Id.Key
}

func (s *Service) removeElementFromSlice(subscribers []string, subscriberID string) {
	var index int
	for subscriberIndex, currentSubscriberID := range subscribers {
		if subscriberID == currentSubscriberID {
			index = subscriberIndex
			break
		}
	}
	subscribers[index] = subscribers[len(subscribers)-1]
	subscribers[len(subscribers)-1] = ""
	subscribers = subscribers[:len(subscribers)-1]
}
