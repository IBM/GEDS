/**
 * Copyright 2023- Pezhman Nasirifard. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

package main

import (
	"context"
	"github.com/IBM/gedsmds/protos"
	"testing"
	"time"
)

func TestMDS_ObjectStore(t *testing.T) {
	ctx := context.Background()
	client, closer := mockServerClient(ctx)
	defer closer()

	type registerObjectStoreResponse struct {
		out *protos.StatusResponse
		err error
	}

	testCreate := map[string]struct {
		in       *protos.ObjectStoreConfig
		expected registerObjectStoreResponse
	}{
		"CreateObjectStore_!": {
			in: &protos.ObjectStoreConfig{
				Bucket:      "bucket1",
				EndpointUrl: "geds://",
				AccessKey:   "access1",
				SecretKey:   "secret1",
			},
			expected: registerObjectStoreResponse{
				out: &protos.StatusResponse{Code: protos.StatusCode_OK},
				err: nil,
			},
		},
	}

	for scenario, test := range testCreate {
		t.Run(scenario, func(t *testing.T) {
			out, err := client.RegisterObjectStore(ctx, test.in)
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

	time.Sleep(100 * time.Millisecond)

	type listObjectStoresResponse struct {
		out *protos.AvailableObjectStoreConfigs
		err error
	}

	testList := map[string]struct {
		in       *protos.EmptyParams
		expected listObjectStoresResponse
	}{
		"ListObjectStore_1": {
			in: &protos.EmptyParams{},
			expected: listObjectStoresResponse{
				out: &protos.AvailableObjectStoreConfigs{
					Mappings: []*protos.ObjectStoreConfig{
						{
							Bucket:      "bucket1",
							EndpointUrl: "geds://",
							AccessKey:   "access1",
							SecretKey:   "secret1",
						},
					},
				},
				err: nil,
			},
		},
	}

	for scenario, test := range testList {
		t.Run(scenario, func(t *testing.T) {
			out, err := client.ListObjectStores(ctx, test.in)
			if err != nil {
				if test.expected.err.Error() != err.Error() {
					t.Errorf("Error -> \nExpected: %q\nReceived: %q\n", test.expected.err, err)
				}
			} else {
				if test.expected.out.Mappings[0].Bucket != out.Mappings[0].Bucket {
					t.Errorf("Not Matching -> \nExpected: %q\nReceived : %q", test.expected.out, out)
				}
			}
		})
	}
}
