/**
 * Copyright 2023- Pezhman Nasirifard. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

package main

import (
	"context"
	"errors"
	"github.com/IBM/gedsmds/internal/logger"
	"github.com/IBM/gedsmds/protos"
	"io"
	"testing"
	"time"
)

func TestMDS_PubSub(t *testing.T) {
	ctx := context.Background()
	client, closer := mockServerClient(ctx)
	defer closer()

	const subscriberId = "uuid1"

	type subscribeResponse struct {
		out *protos.StatusResponse
		err error
	}

	testSubscribeUnsubscribe := map[string]struct {
		in       *protos.SubscriptionEvent
		expected subscribeResponse
	}{
		"SubscribeUnsubscribe_1": {
			in: &protos.SubscriptionEvent{
				SubscriberID:     subscriberId,
				BucketID:         "bucket3",
				Key:              "photos/2006/february/sample1.jpg",
				SubscriptionType: protos.SubscriptionType_OBJECT,
			},
			expected: subscribeResponse{
				out: &protos.StatusResponse{Code: protos.StatusCode_OK},
				err: nil,
			},
		},
		"SubscribeUnsubscribe_2": {
			in: &protos.SubscriptionEvent{
				SubscriberID:     subscriberId,
				BucketID:         "bucket3",
				SubscriptionType: protos.SubscriptionType_BUCKET,
			},
			expected: subscribeResponse{
				out: &protos.StatusResponse{Code: protos.StatusCode_OK},
				err: nil,
			},
		},
		"SubscribeUnsubscribe_3": {
			in: &protos.SubscriptionEvent{
				SubscriberID:     subscriberId,
				BucketID:         "bucket3",
				Key:              "photos/2006/february/",
				SubscriptionType: protos.SubscriptionType_PREFIX,
			},
			expected: subscribeResponse{
				out: &protos.StatusResponse{Code: protos.StatusCode_OK},
				err: nil,
			},
		},
	}

	for scenario, test := range testSubscribeUnsubscribe {
		t.Run(scenario, func(t *testing.T) {
			out, err := client.Subscribe(ctx, test.in)
			if err != nil {
				if test.expected.err.Error() != err.Error() {
					t.Errorf("Error -> \nExpected: %q\nReceived: %q\n", test.expected.err, err)
				}
			} else {
				if test.expected.out.Code != out.Code {
					t.Errorf("Not Matching -> \nExpected: %q\nReceived : %q", test.expected.out, out)
				}
			}
		})
	}

	type streamResponse struct {
		out []*protos.SubscriptionStreamResponse
		err error
	}

	testStream := map[string]struct {
		in       *protos.SubscriptionStreamEvent
		expected streamResponse
	}{
		"SubscribeStream_1": {
			in: &protos.SubscriptionStreamEvent{
				SubscriberID: subscriberId,
			},
			expected: streamResponse{
				out: []*protos.SubscriptionStreamResponse{
					{
						Object: &protos.Object{
							Id: &protos.ObjectID{
								Bucket: "bucket3",
								Key:    "sample.jpg",
							},
							Info: &protos.ObjectInfo{
								Location:     "geds://location1",
								Size:         4000,
								SealedOffset: 4000,
							},
						},
						PublicationType: protos.PublicationType_CREATE_OBJECT,
					}, {
						Object: &protos.Object{
							Id: &protos.ObjectID{
								Bucket: "bucket3",
								Key:    "photos/2006/january/sample.jpg",
							},
							Info: &protos.ObjectInfo{
								Location:     "geds://location2",
								Size:         4000,
								SealedOffset: 4000,
							},
						},
						PublicationType: protos.PublicationType_CREATE_OBJECT,
					}, {
						Object: &protos.Object{
							Id: &protos.ObjectID{
								Bucket: "bucket3",
								Key:    "photos/2006/february/sample2.jpg",
							},
							Info: &protos.ObjectInfo{
								Location:     "geds://location3",
								Size:         4000,
								SealedOffset: 4000,
							},
						},
						PublicationType: protos.PublicationType_CREATE_OBJECT,
					}, {
						Object: &protos.Object{
							Id: &protos.ObjectID{
								Bucket: "bucket3",
								Key:    "photos/2006/february/sample3.jpg",
							},
							Info: &protos.ObjectInfo{
								Location:     "geds://location4",
								Size:         4000,
								SealedOffset: 4000,
							},
						},
						PublicationType: protos.PublicationType_CREATE_OBJECT,
					}, {
						Object: &protos.Object{
							Id: &protos.ObjectID{
								Bucket: "bucket3",
								Key:    "photos/2006/february/sample4.jpg",
							},
							Info: &protos.ObjectInfo{
								Location:     "geds://location5",
								Size:         4000,
								SealedOffset: 4000,
							},
						},
						PublicationType: protos.PublicationType_CREATE_OBJECT,
					},
				},
				err: nil,
			},
		},
	}

	for scenario, tt := range testStream {
		t.Run(scenario, func(t *testing.T) {
			out, err := client.SubscribeStream(ctx, tt.in)
			go sendPublications(client)
			var responses []*protos.SubscriptionStreamResponse
			for {
				response, errResponse := out.Recv()
				if errors.Is(errResponse, io.EOF) {
					break
				}
				responses = append(responses, response)
				if len(responses) == len(tt.expected.out) {
					break
				}
			}

			if err != nil {
				if tt.expected.err.Error() != err.Error() {
					t.Errorf("Error -> \nExpected: %q\nReceived: %q\n", tt.expected.err, err)
				}
			} else {
				if len(responses) != len(tt.expected.out) {
					t.Errorf("Not Matching -> \nExpected: %q\nReceived : %q", tt.expected.out, responses)
				}
			}
		})
	}

	for scenario, test := range testSubscribeUnsubscribe {
		t.Run(scenario, func(t *testing.T) {
			out, err := client.Unsubscribe(ctx, test.in)
			if err != nil {
				if test.expected.err.Error() != err.Error() {
					t.Errorf("Error -> \nExpected: %q\nReceived: %q\n", test.expected.err, err)
				}
			} else {
				if test.expected.out.Code != out.Code {
					t.Errorf("Not Matching -> \nExpected: %q\nReceived : %q", test.expected.out, out)
				}
			}
		})
	}

}

func sendPublications(client protos.MetadataServiceClient) {
	time.Sleep(1000 * time.Millisecond)
	ctx := context.Background()
	objectsToCreate := []*protos.Object{
		{
			Id: &protos.ObjectID{
				Bucket: "bucket3",
				Key:    "sample.jpg",
			},
			Info: &protos.ObjectInfo{
				Location:     "geds://location1",
				Size:         4000,
				SealedOffset: 4000,
			},
		},
		{
			Id: &protos.ObjectID{
				Bucket: "bucket3",
				Key:    "photos/2006/january/sample.jpg",
			},
			Info: &protos.ObjectInfo{
				Location:     "geds://location2",
				Size:         4000,
				SealedOffset: 4000,
			},
		},
		{
			Id: &protos.ObjectID{
				Bucket: "bucket3",
				Key:    "photos/2006/february/sample2.jpg",
			},
			Info: &protos.ObjectInfo{
				Location:     "geds://location3",
				Size:         4000,
				SealedOffset: 4000,
			},
		},
		{
			Id: &protos.ObjectID{
				Bucket: "bucket3",
				Key:    "photos/2006/february/sample3.jpg",
			},
			Info: &protos.ObjectInfo{
				Location:     "geds://location4",
				Size:         4000,
				SealedOffset: 4000,
			},
		},
		{
			Id: &protos.ObjectID{
				Bucket: "bucket3",
				Key:    "photos/2006/february/sample4.jpg",
			},
			Info: &protos.ObjectInfo{
				Location:     "geds://location5",
				Size:         4000,
				SealedOffset: 4000,
			},
		},
	}

	for _, object := range objectsToCreate {
		response, err := client.Create(ctx, object)
		if err != nil || response.Code != protos.StatusCode_OK {
			logger.ErrorLogger.Println("the publication could not be sent")
		}
	}
}
