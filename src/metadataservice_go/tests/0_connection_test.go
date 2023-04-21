package main

import (
	"context"
	"github.com/IBM/gedsmds/protos"
	"testing"
)

func TestMDS_Connection(t *testing.T) {
	ctx := context.Background()
	client, closer := mockServerClient(ctx)
	defer closer()

	type getConnectionInfoResponse struct {
		out *protos.ConnectionInformation
		err error
	}

	testConnection := map[string]struct {
		in       *protos.EmptyParams
		expected getConnectionInfoResponse
	}{
		"GetConnectionInfo_1": {
			in: &protos.EmptyParams{},
			expected: getConnectionInfoResponse{
				out: &protos.ConnectionInformation{RemoteAddress: "bufconn"},
				err: nil,
			},
		},
	}

	for scenario, test := range testConnection {
		t.Run(scenario, func(t *testing.T) {
			out, err := client.GetConnectionInformation(ctx, test.in)
			if err != nil {
				if test.expected.err.Error() != err.Error() {
					t.Errorf("Err -> \nWant: %q\nGot: %q\n", test.expected.err, err)
				}
			} else {
				if test.expected.out.RemoteAddress != out.RemoteAddress {
					t.Errorf("Out -> \nWant: %q\nGot : %q", test.expected.out, out)
				}
			}
		})
	}

}
