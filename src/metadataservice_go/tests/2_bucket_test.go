/**
 * Copyright 2023- Technical University of Munich. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

package main

import (
	"context"
	"github.com/IBM/gedsmds/protos"
	"testing"
	"time"
)

func TestMDS_Bucket(t *testing.T) {
	ctx := context.Background()
	client, closer := mockServerClient(ctx)
	defer closer()

	type bucketResponse struct {
		out *protos.StatusResponse
		err error
	}

	testCreateLookUpDelete := map[string]struct {
		in       *protos.Bucket
		expected bucketResponse
	}{
		"CreateLookUpDeleteBucket_1": {
			in: &protos.Bucket{
				Bucket: "bucket1",
			},
			expected: bucketResponse{
				out: &protos.StatusResponse{Code: protos.StatusCode_OK},
				err: nil,
			},
		},
	}

	for scenario, test := range testCreateLookUpDelete {
		t.Run(scenario, func(t *testing.T) {
			out, err := client.CreateBucket(ctx, test.in)
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
	for scenario, test := range testCreateLookUpDelete {
		t.Run(scenario, func(t *testing.T) {
			out, err := client.LookupBucket(ctx, test.in)
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

	type listBucketsResponse struct {
		out *protos.BucketListResponse
		err error
	}

	testList := map[string]struct {
		in       *protos.EmptyParams
		expected listBucketsResponse
	}{
		"ListBucket_1": {
			in: &protos.EmptyParams{},
			expected: listBucketsResponse{
				out: &protos.BucketListResponse{
					Results: []string{"bucket1"},
				},
				err: nil,
			},
		},
	}

	for scenario, test := range testList {
		t.Run(scenario, func(t *testing.T) {
			out, err := client.ListBuckets(ctx, test.in)
			if err != nil {
				if test.expected.err.Error() != err.Error() {
					t.Errorf("Error -> \nExpected: %q\nReceived: %q\n", test.expected.err, err)
				}
			} else {
				if test.expected.out.Results[0] != out.Results[0] {
					t.Errorf("Not Matching -> \nExpected: %q\nReceived : %q", test.expected.out, out)
				}
			}
		})
	}

	for scenario, test := range testCreateLookUpDelete {
		t.Run(scenario, func(t *testing.T) {
			out, err := client.DeleteBucket(ctx, test.in)
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
