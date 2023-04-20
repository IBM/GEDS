package main

import (
	"context"
	"github.com/IBM/gedsmds/protos"
	"testing"
)

func TestMDS_Objects(t *testing.T) {
	ctx := context.Background()
	client, closer := mockServer(ctx)
	defer closer()

	type objectResponse struct {
		out *protos.StatusResponse
		err error
	}

	testCreate := map[string]struct {
		in       *protos.Object
		expected objectResponse
	}{
		"Success_1": {
			in: &protos.Object{
				Id: &protos.ObjectID{
					Bucket: "bucket2",
					Key:    "sample.jpg",
				},
				Info: &protos.ObjectInfo{
					Location:     "geds://location1",
					Size:         4000,
					SealedOffset: 4000,
				},
			},
			expected: objectResponse{
				out: &protos.StatusResponse{Code: protos.StatusCode_OK},
				err: nil,
			},
		},
		"Success_2": {
			in: &protos.Object{
				Id: &protos.ObjectID{
					Bucket: "bucket2",
					Key:    "photos/2006/January/sample.jpg",
				},
				Info: &protos.ObjectInfo{
					Location:     "geds://location2",
					Size:         4000,
					SealedOffset: 4000,
				},
			},
			expected: objectResponse{
				out: &protos.StatusResponse{Code: protos.StatusCode_OK},
				err: nil,
			},
		},
		"Success_3": {
			in: &protos.Object{
				Id: &protos.ObjectID{
					Bucket: "bucket2",
					Key:    "photos/2006/February/sample2.jpg",
				},
				Info: &protos.ObjectInfo{
					Location:     "geds://location3",
					Size:         4000,
					SealedOffset: 4000,
				},
			},
			expected: objectResponse{
				out: &protos.StatusResponse{Code: protos.StatusCode_OK},
				err: nil,
			},
		},
		"Success_4": {
			in: &protos.Object{
				Id: &protos.ObjectID{
					Bucket: "bucket2",
					Key:    "photos/2006/February/sample3.jpg",
				},
				Info: &protos.ObjectInfo{
					Location:     "geds://location4",
					Size:         4000,
					SealedOffset: 4000,
				},
			},
			expected: objectResponse{
				out: &protos.StatusResponse{Code: protos.StatusCode_OK},
				err: nil,
			},
		},
		"Success_5": {
			in: &protos.Object{
				Id: &protos.ObjectID{
					Bucket: "bucket2",
					Key:    "photos/2006/February/sample4.jpg",
				},
				Info: &protos.ObjectInfo{
					Location:     "geds://location5",
					Size:         4000,
					SealedOffset: 4000,
				},
			},
			expected: objectResponse{
				out: &protos.StatusResponse{Code: protos.StatusCode_OK},
				err: nil,
			},
		},
	}

	for scenario, test := range testCreate {
		t.Run(scenario, func(t *testing.T) {
			out, err := client.Create(ctx, test.in)
			if err != nil {
				if test.expected.err.Error() != err.Error() {
					t.Errorf("Err -> \nWant: %q\nGot: %q\n", test.expected.err, err)
				}
			} else {
				if test.expected.out.Code != out.Code {
					t.Errorf("Out -> \nWant: %q\nGot : %q", test.expected.out, out)
				}
			}
		})
	}

	//type objectListResponse struct {
	//	out *protos.ObjectListResponse
	//	err error
	//}
	//
	//testList := map[string]struct {
	//	in       *protos.ObjectListRequest
	//	expected objectListResponse
	//}{
	//	"List_1": {
	//		in: &protos.ObjectListRequest{
	//			Prefix: &protos.ObjectID{
	//				Bucket: "bucket2",
	//				Key:    "photos/2006/February",
	//			},
	//		},
	//		expected: objectListResponse{
	//			out: &protos.ObjectListResponse{},
	//			err: nil,
	//		},
	//	},
	//	"List_2": {
	//		in: &protos.ObjectListRequest{},
	//		expected: objectListResponse{
	//			out: &protos.ObjectListResponse{},
	//			err: nil,
	//		},
	//	},
	//	"List_3": {
	//		in: &protos.ObjectListRequest{},
	//		expected: objectListResponse{
	//			out: &protos.ObjectListResponse{},
	//			err: nil,
	//		},
	//	},
	//}
	//
	//for scenario, test := range testList {
	//	t.Run(scenario, func(t *testing.T) {
	//		out, err := client.List(ctx, test.in)
	//		if err != nil {
	//			if test.expected.err.Error() != err.Error() {
	//				t.Errorf("Err -> \nWant: %q\nGot: %q\n", test.expected.err, err)
	//			}
	//		} else {
	//			if test.expected.out.Code != out.Code {
	//				t.Errorf("Out -> \nWant: %q\nGot : %q", test.expected.out, out)
	//			}
	//		}
	//	})
	//}

}
