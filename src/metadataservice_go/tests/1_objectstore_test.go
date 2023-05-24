/**
 * Copyright 2023- Pezhman Nasirifard. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

package main

import (
	"context"
	"github.com/IBM/gedsmds/internal/logger"
	"github.com/IBM/gedsmds/protos"
	"os"
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
		"CreateObjectStore_1": {
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

	s3BucketTest := os.Getenv("S3_BUCKET_TEST")
	s3EndpointTest := os.Getenv("S3_ENDPOINT_TEST")
	s3AccessKeyTest := os.Getenv("S3_ACCESS_KEY_TEST")
	s3SecretKeyTest := os.Getenv("S3_SECRET_KEY_TEST")

	if len(s3EndpointTest) == 0 || len(s3BucketTest) == 0 || len(s3AccessKeyTest) == 0 || len(s3SecretKeyTest) == 0 {
		logger.ErrorLogger.Println("S3 env vars are not set in env file for populating key-value store is not set." +
			" Skipping this test")
	} else {
		testCreate = map[string]struct {
			in       *protos.ObjectStoreConfig
			expected registerObjectStoreResponse
		}{
			"CreateObjectStore_2": {
				in: &protos.ObjectStoreConfig{
					Bucket:      s3BucketTest,
					EndpointUrl: s3EndpointTest,
					AccessKey:   s3AccessKeyTest,
					SecretKey:   s3SecretKeyTest,
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

		time.Sleep(5 * time.Second)

		type objectListResponse struct {
			out *protos.ObjectListResponse
			err error
		}

		// only testing if a minimum number of objects have been retrieved from S3.
		testListObject := map[string]struct {
			in       *protos.ObjectListRequest
			expected objectListResponse
		}{
			"ListObject_1": {
				in: &protos.ObjectListRequest{
					Prefix: &protos.ObjectID{
						Bucket: s3BucketTest,
					},
				},
				expected: objectListResponse{
					out: &protos.ObjectListResponse{
						Results: []*protos.Object{
							{
								Id: &protos.ObjectID{
									Bucket: s3BucketTest,
									Key:    "mock_1",
								},
							},
							{
								Id: &protos.ObjectID{
									Bucket: s3BucketTest,
									Key:    "mock_2",
								},
							},
							{
								Id: &protos.ObjectID{
									Bucket: s3BucketTest,
									Key:    "mock_3",
								},
							},
							{
								Id: &protos.ObjectID{
									Bucket: s3BucketTest,
									Key:    "mock_4",
								},
							},
							{
								Id: &protos.ObjectID{
									Bucket: s3BucketTest,
									Key:    "mock_5",
								},
							},
						},
						CommonPrefixes: []string{},
					},
					err: nil,
				},
			},
		}

		for scenario, test := range testListObject {
			t.Run(scenario, func(t *testing.T) {
				out, err := client.List(ctx, test.in)
				if err != nil {
					if test.expected.err.Error() != err.Error() {
						t.Errorf("Error -> \nExpected: %q\nReceived: %q\n", test.expected.err, err)
					}
				} else {
					if len(test.expected.out.Results) > len(out.Results) {
						t.Errorf("Not Matching -> \nExpected: %q\nReceived : %q", test.expected.out, out)
					}
				}
			})
		}

	}

}
