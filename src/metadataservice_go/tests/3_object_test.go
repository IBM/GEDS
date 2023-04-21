package main

import (
	"context"
	"github.com/IBM/gedsmds/protos"
	"testing"
	"time"
)

func TestMDS_Objects(t *testing.T) {
	ctx := context.Background()
	client, closer := mockServerClient(ctx)
	defer closer()

	type objectResponse struct {
		out *protos.StatusResponse
		err error
	}

	testCreate := map[string]struct {
		in       *protos.Object
		expected objectResponse
	}{
		"CreateObject_1": {
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
		"CreateObject_2": {
			in: &protos.Object{
				Id: &protos.ObjectID{
					Bucket: "bucket2",
					Key:    "photos/2006/january/sample.jpg",
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
		"CreateObject_3": {
			in: &protos.Object{
				Id: &protos.ObjectID{
					Bucket: "bucket2",
					Key:    "photos/2006/february/sample2.jpg",
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
		"CreateObject_4": {
			in: &protos.Object{
				Id: &protos.ObjectID{
					Bucket: "bucket2",
					Key:    "photos/2006/february/sample3.jpg",
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
		"CreateObject_5": {
			in: &protos.Object{
				Id: &protos.ObjectID{
					Bucket: "bucket2",
					Key:    "photos/2006/february/sample4.jpg",
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

	time.Sleep(100 * time.Millisecond)

	type objectListResponse struct {
		out *protos.ObjectListResponse
		err error
	}

	delimiter := int32(47)
	testList := map[string]struct {
		in       *protos.ObjectListRequest
		expected objectListResponse
	}{
		"ListObject_1": {
			in: &protos.ObjectListRequest{
				Prefix: &protos.ObjectID{
					Bucket: "bucket2",
					Key:    "photos/2006/february/",
				},
			},
			expected: objectListResponse{
				out: &protos.ObjectListResponse{
					Results: []*protos.Object{
						{
							Id: &protos.ObjectID{
								Bucket: "bucket2",
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
								Bucket: "bucket2",
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
								Bucket: "bucket2",
								Key:    "photos/2006/february/sample4.jpg",
							},
							Info: &protos.ObjectInfo{
								Location:     "geds://location5",
								Size:         4000,
								SealedOffset: 4000,
							},
						},
					},
					CommonPrefixes: []string{},
				},
				err: nil,
			},
		},
		"ListObject_2": {
			in: &protos.ObjectListRequest{
				Prefix: &protos.ObjectID{
					Bucket: "bucket2",
				},
				Delimiter: &delimiter,
			},
			expected: objectListResponse{
				out: &protos.ObjectListResponse{
					Results: []*protos.Object{
						{
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
					},
					CommonPrefixes: []string{"photos/"},
				},
				err: nil,
			},
		},
		"ListObject_3": {
			in: &protos.ObjectListRequest{
				Prefix: &protos.ObjectID{
					Bucket: "bucket2",
					Key:    "photos/2006/",
				},
				Delimiter: &delimiter,
			},
			expected: objectListResponse{
				out: &protos.ObjectListResponse{
					Results: []*protos.Object{
						{
							Id: &protos.ObjectID{
								Bucket: "bucket2",
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
								Bucket: "bucket2",
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
								Bucket: "bucket2",
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
								Bucket: "bucket2",
								Key:    "photos/2006/february/sample4.jpg",
							},
							Info: &protos.ObjectInfo{
								Location:     "geds://location5",
								Size:         4000,
								SealedOffset: 4000,
							},
						},
					},
					CommonPrefixes: []string{"photos/2006/january/", "photos/2006/february/"},
				},
				err: nil,
			},
		},
	}

	for scenario, test := range testList {
		t.Run(scenario, func(t *testing.T) {
			out, err := client.List(ctx, test.in)
			if err != nil {
				if test.expected.err.Error() != err.Error() {
					t.Errorf("Err -> \nWant: %q\nGot: %q\n", test.expected.err, err)
				}
			} else {
				if len(test.expected.out.Results) != len(out.Results) ||
					len(test.expected.out.CommonPrefixes) != len(out.CommonPrefixes) {
					t.Errorf("Out -> \nWant: %q\nGot : %q", test.expected.out, out)
				}
			}
		})
	}

	type objectLookupResponse struct {
		out *protos.Object
		err error
	}

	testLookup := map[string]struct {
		in       *protos.ObjectID
		expected *objectLookupResponse
	}{
		"ObjectLookUp_1": {
			in: &protos.ObjectID{
				Bucket: "bucket2",
				Key:    "photos/2006/february/sample4.jpg",
			},
			expected: &objectLookupResponse{
				out: &protos.Object{
					Id: &protos.ObjectID{
						Bucket: "bucket2",
						Key:    "photos/2006/february/sample4.jpg",
					},
					Info: &protos.ObjectInfo{
						Location:     "geds://location5",
						Size:         4000,
						SealedOffset: 4000,
					},
				},
				err: nil,
			},
		},
	}

	for scenario, test := range testLookup {
		t.Run(scenario, func(t *testing.T) {
			out, err := client.Lookup(ctx, test.in)
			if err != nil {
				if test.expected.err.Error() != err.Error() {
					t.Errorf("Err -> \nWant: %q\nGot: %q\n", test.expected.err, err)
				}
			} else {
				if len(test.expected.out.Info.Location) != len(out.Result.Info.Location) {
					t.Errorf("Out -> \nWant: %q\nGot : %q", test.expected.out, out)
				}
			}
		})
	}

	testDelete := map[string]struct {
		in       *protos.ObjectID
		expected *objectResponse
	}{
		"DeleteObject_1": {
			in: &protos.ObjectID{
				Bucket: "bucket2",
				Key:    "photos/2006/february/sample4.jpg",
			},
			expected: &objectResponse{
				out: &protos.StatusResponse{Code: protos.StatusCode_OK},
				err: nil,
			},
		},
	}

	for scenario, test := range testDelete {
		t.Run(scenario, func(t *testing.T) {
			out, err := client.Delete(ctx, test.in)
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

	testDeletePrefix := map[string]struct {
		in       *protos.ObjectID
		expected *objectResponse
	}{
		"DeleteObjectPrefix_1": {
			in: &protos.ObjectID{
				Bucket: "bucket2",
				Key:    "photos/2006/february/",
			},
			expected: &objectResponse{
				out: &protos.StatusResponse{Code: protos.StatusCode_OK},
				err: nil,
			},
		},
	}

	for scenario, test := range testDeletePrefix {
		t.Run(scenario, func(t *testing.T) {
			out, err := client.DeletePrefix(ctx, test.in)
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

	testUpdate := map[string]struct {
		in       *protos.Object
		expected *objectResponse
	}{
		"UpdateObject_1": {
			in: &protos.Object{
				Id: &protos.ObjectID{
					Bucket: "bucket2",
					Key:    "photos/2006/january/sample.jpg",
				},
				Info: &protos.ObjectInfo{
					Location:     "geds://location2-updated",
					Size:         5000,
					SealedOffset: 5000,
				},
			},
			expected: &objectResponse{
				out: &protos.StatusResponse{Code: protos.StatusCode_OK},
				err: nil,
			},
		},
	}

	for scenario, test := range testUpdate {
		t.Run(scenario, func(t *testing.T) {
			out, err := client.Update(ctx, test.in)
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

	testList = map[string]struct {
		in       *protos.ObjectListRequest
		expected objectListResponse
	}{
		"ListObject_4": {
			in: &protos.ObjectListRequest{
				Prefix: &protos.ObjectID{
					Bucket: "bucket2",
					Key:    "photos/2006/",
				},
			},
			expected: objectListResponse{
				out: &protos.ObjectListResponse{
					Results: []*protos.Object{
						{
							Id: &protos.ObjectID{
								Bucket: "bucket2",
								Key:    "photos/2006/january/sample.jpg",
							},
							Info: &protos.ObjectInfo{
								Location:     "geds://location2",
								Size:         4000,
								SealedOffset: 4000,
							},
						},
					},
					CommonPrefixes: []string{},
				},
				err: nil,
			},
		},
	}

	for scenario, test := range testList {
		t.Run(scenario, func(t *testing.T) {
			out, err := client.List(ctx, test.in)
			if err != nil {
				if test.expected.err.Error() != err.Error() {
					t.Errorf("Err -> \nWant: %q\nGot: %q\n", test.expected.err, err)
				}
			} else {
				if len(test.expected.out.Results) != len(out.Results) ||
					len(test.expected.out.CommonPrefixes) != len(out.CommonPrefixes) {
					t.Errorf("Out -> \nWant: %q\nGot : %q", test.expected.out, out)
				}
			}
		})
	}

}
