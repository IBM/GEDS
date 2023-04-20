package main

import (
	"context"
	"github.com/IBM/gedsmds/protos"
	"testing"
)

func TestMDS_ObjectStore(t *testing.T) {
	ctx := context.Background()
	client, closer := mockServer(ctx)
	defer closer()

	type registerObjectStoreResponse struct {
		out *protos.StatusResponse
		err error
	}

	testCreate := map[string]struct {
		in       *protos.ObjectStoreConfig
		expected registerObjectStoreResponse
	}{
		"Success": {
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
					t.Errorf("Err -> \nWant: %q\nGot: %q\n", test.expected.err, err)
				}
			} else {
				if test.expected.out.Code != out.Code {
					t.Errorf("Out -> \nWant: %q\nGot : %q", test.expected.out, out)
				}
			}
		})
	}

	type listObjectStoresResponse struct {
		out *protos.AvailableObjectStoreConfigs
		err error
	}

	testList := map[string]struct {
		in       *protos.EmptyParams
		expected listObjectStoresResponse
	}{
		"Success": {
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
					t.Errorf("Err -> \nWant: %q\nGot: %q\n", test.expected.err, err)
				}
			} else {
				if test.expected.out.Mappings[0].Bucket != out.Mappings[0].Bucket {
					t.Errorf("Out -> \nWant: %q\nGot : %q", test.expected.out, out)
				}
			}
		})
	}
}
