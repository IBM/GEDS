package mockgedsclient

import (
	"context"
	"github.com/IBM/gedsmds/internal/connection/connpool"
	"github.com/IBM/gedsmds/internal/logger"
	"github.com/IBM/gedsmds/protos"
	"io"
)

type Executor struct {
	mdsConnections map[string]*connpool.Pool
}

func NewExecutor() *Executor {
	tempEx := &Executor{
		mdsConnections: connpool.GetMDSConnectionsStream(),
	}
	connpool.SleepAndContinue()
	return tempEx
}

func (e *Executor) RegisterObjectStore() {
	conn, err := e.mdsConnections["127.0.0.1"].Get(context.Background())
	if conn == nil || err != nil {
		logger.ErrorLogger.Println(err)
	}
	client := protos.NewMetadataServiceClient(conn.ClientConn)
	oStore := &protos.ObjectStoreConfig{
		Bucket:      "b1",
		AccessKey:   "key1",
		EndpointUrl: "geds://",
		SecretKey:   "secKey",
	}
	result, err := client.RegisterObjectStore(context.Background(), oStore)
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	oStore = &protos.ObjectStoreConfig{
		Bucket:      "b2",
		AccessKey:   "key2",
		EndpointUrl: "geds://",
		SecretKey:   "secKey2",
	}
	result, err = client.RegisterObjectStore(context.Background(), oStore)
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result.Code)
	if errCon := conn.Close(); errCon != nil {
		logger.ErrorLogger.Println(errCon)
	}
}

func (e *Executor) ListObjectStore() {
	conn, err := e.mdsConnections["127.0.0.1"].Get(context.Background())
	if conn == nil || err != nil {
		logger.ErrorLogger.Println(err)
	}
	client := protos.NewMetadataServiceClient(conn.ClientConn)
	result, err := client.ListObjectStores(context.Background(), &protos.EmptyParams{})
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result.Mappings)
	if errCon := conn.Close(); errCon != nil {
		logger.ErrorLogger.Println(errCon)
	}
}

func (e *Executor) CreateBucket() {
	conn, err := e.mdsConnections["127.0.0.1"].Get(context.Background())
	if conn == nil || err != nil {
		logger.ErrorLogger.Println(err)
	}
	client := protos.NewMetadataServiceClient(conn.ClientConn)
	bucket := &protos.Bucket{
		Bucket: "b1",
	}
	result, err := client.CreateBucket(context.Background(), bucket)
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	bucket = &protos.Bucket{
		Bucket: "b2",
	}
	result, err = client.CreateBucket(context.Background(), bucket)
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result.Code)
	if errCon := conn.Close(); errCon != nil {
		logger.ErrorLogger.Println(errCon)
	}
}

func (e *Executor) ListBuckets() {
	conn, err := e.mdsConnections["127.0.0.1"].Get(context.Background())
	if conn == nil || err != nil {
		logger.ErrorLogger.Println(err)
	}
	client := protos.NewMetadataServiceClient(conn.ClientConn)
	result, err := client.ListBuckets(context.Background(), &protos.EmptyParams{})
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result.Results)
	if errCon := conn.Close(); errCon != nil {
		logger.ErrorLogger.Println(errCon)
	}
}

func (e *Executor) DeleteBucket() {
	conn, err := e.mdsConnections["127.0.0.1"].Get(context.Background())
	if conn == nil || err != nil {
		logger.ErrorLogger.Println(err)
	}
	client := protos.NewMetadataServiceClient(conn.ClientConn)
	bucket := &protos.Bucket{
		Bucket: "b1",
	}
	result, err := client.DeleteBucket(context.Background(), bucket)
	logger.InfoLogger.Println(result.Code)
	if errCon := conn.Close(); errCon != nil {
		logger.ErrorLogger.Println(errCon)
	}
}

func (e *Executor) LookUpBucket() {
	conn, err := e.mdsConnections["127.0.0.1"].Get(context.Background())
	if conn == nil || err != nil {
		logger.ErrorLogger.Println(err)
	}
	client := protos.NewMetadataServiceClient(conn.ClientConn)
	bucket := &protos.Bucket{
		Bucket: "b1",
	}
	result, err := client.LookupBucket(context.Background(), bucket)
	logger.InfoLogger.Println(result.Code)
	if errCon := conn.Close(); errCon != nil {
		logger.ErrorLogger.Println(errCon)
	}
}

func (e *Executor) CreateObject() {
	conn, err := e.mdsConnections["127.0.0.1"].Get(context.Background())
	if conn == nil || err != nil {
		logger.ErrorLogger.Println(err)
	}
	client := protos.NewMetadataServiceClient(conn.ClientConn)
	object := &protos.Object{
		Id: &protos.ObjectID{
			Key:    "sample.jpg",
			Bucket: "b2",
		},
		Info: &protos.ObjectInfo{
			Location:     "here3",
			Size:         4000,
			SealedOffset: 4000,
		},
	}
	result, err := client.Create(context.Background(), object)
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result.Code)
	if errCon := conn.Close(); errCon != nil {
		logger.ErrorLogger.Println(errCon)
	}

	object = &protos.Object{
		Id: &protos.ObjectID{
			Key:    "photos/2006/January/sample.jpg",
			Bucket: "b2",
		},
		Info: &protos.ObjectInfo{
			Location:     "here3",
			Size:         4000,
			SealedOffset: 4000,
		},
	}
	result, err = client.Create(context.Background(), object)
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result.Code)
	if errCon := conn.Close(); errCon != nil {
		//logger.ErrorLogger.Println(errCon)
	}

	object = &protos.Object{
		Id: &protos.ObjectID{
			Key:    "photos/2006/February/sample2.jpg",
			Bucket: "b2",
		},
		Info: &protos.ObjectInfo{
			Location:     "here3",
			Size:         4000,
			SealedOffset: 4000,
		},
	}
	result, err = client.Create(context.Background(), object)
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result.Code)
	if errCon := conn.Close(); errCon != nil {
		//logger.ErrorLogger.Println(errCon)
	}

	object = &protos.Object{
		Id: &protos.ObjectID{
			Key:    "photos/2006/February/sample3.jpg",
			Bucket: "b2",
		},
		Info: &protos.ObjectInfo{
			Location:     "here3",
			Size:         4000,
			SealedOffset: 4000,
		},
	}
	result, err = client.Create(context.Background(), object)
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result.Code)
	if errCon := conn.Close(); errCon != nil {
		//logger.ErrorLogger.Println(errCon)
	}
	object = &protos.Object{
		Id: &protos.ObjectID{
			Key:    "photos/2006/February/sample4.jpg",
			Bucket: "b2",
		},
		Info: &protos.ObjectInfo{
			Location:     "here3",
			Size:         4000,
			SealedOffset: 4000,
		},
	}
	result, err = client.Create(context.Background(), object)
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result.Code)
	if errCon := conn.Close(); errCon != nil {
		//logger.ErrorLogger.Println(errCon)
	}
}

func (e *Executor) UpdateObject() {
	conn, err := e.mdsConnections["127.0.0.1"].Get(context.Background())
	if conn == nil || err != nil {
		logger.ErrorLogger.Println(err)
	}
	client := protos.NewMetadataServiceClient(conn.ClientConn)
	object := &protos.Object{
		Id: &protos.ObjectID{
			Key:    "photos/2006/February/sample4.jpg",
			Bucket: "b2",
		},
		Info: &protos.ObjectInfo{
			Location:     "here2-updated",
			Size:         3000,
			SealedOffset: 3000,
		},
	}
	result, err := client.Update(context.Background(), object)
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result.Code)
	if errCon := conn.Close(); errCon != nil {
		logger.ErrorLogger.Println(errCon)
	}
}

func (e *Executor) DeleteObject() {
	conn, err := e.mdsConnections["127.0.0.1"].Get(context.Background())
	if conn == nil || err != nil {
		logger.ErrorLogger.Println(err)
	}
	client := protos.NewMetadataServiceClient(conn.ClientConn)
	objectId := &protos.ObjectID{
		Key:    "photos/2006/February/sample3.jpg",
		Bucket: "b2",
	}
	result, err := client.Delete(context.Background(), objectId)
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result.Code)
	if errCon := conn.Close(); errCon != nil {
		logger.ErrorLogger.Println(errCon)
	}
}

func (e *Executor) DeletePrefix() {
	conn, err := e.mdsConnections["127.0.0.1"].Get(context.Background())
	if conn == nil || err != nil {
		logger.ErrorLogger.Println(err)
	}
	client := protos.NewMetadataServiceClient(conn.ClientConn)
	objectId := &protos.ObjectID{
		Key:    "photos/2006/February/",
		Bucket: "b2",
	}
	result, err := client.DeletePrefix(context.Background(), objectId)
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result.Code)
	if errCon := conn.Close(); errCon != nil {
		logger.ErrorLogger.Println(errCon)
	}
}

func (e *Executor) Lookup() {
	conn, err := e.mdsConnections["127.0.0.1"].Get(context.Background())
	if conn == nil || err != nil {
		logger.ErrorLogger.Println(err)
	}
	client := protos.NewMetadataServiceClient(conn.ClientConn)
	objectId := &protos.ObjectID{
		Key:    "sample.jpg",
		Bucket: "b2",
	}
	result, err := client.Lookup(context.Background(), objectId)
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result.Result)

	objectId2 := &protos.ObjectID{
		Key:    "photos/2006/February/sample3.jpg",
		Bucket: "b2",
	}
	result, err = client.Lookup(context.Background(), objectId2)
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result.Result)

	if errCon := conn.Close(); errCon != nil {
		logger.ErrorLogger.Println(errCon)
	}
}

func (e *Executor) ListObjects() {
	conn, err := e.mdsConnections["127.0.0.1"].Get(context.Background())
	if conn == nil || err != nil {
		logger.ErrorLogger.Println(err)
	}
	client := protos.NewMetadataServiceClient(conn.ClientConn)
	result, err := client.List(context.Background(), &protos.ObjectListRequest{
		Prefix: &protos.ObjectID{
			Bucket: "b2",
		},
	})
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result.Results)
	if errCon := conn.Close(); errCon != nil {
		logger.ErrorLogger.Println(errCon)
	}
}

const subscriberId = "uuid1"

func (e *Executor) Subscribe() {
	conn, err := e.mdsConnections["127.0.0.1"].Get(context.Background())
	if conn == nil || err != nil {
		logger.ErrorLogger.Println(err)
	}
	client := protos.NewMetadataServiceClient(conn.ClientConn)

	object := &protos.SubscriptionEvent{
		SubscriberID:     subscriberId,
		Key:              "file3",
		BucketID:         "b2",
		SubscriptionType: protos.SubscriptionType_OBJECT,
	}
	result, err := client.Subscribe(context.Background(), object)
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result.Code)

	object2 := &protos.SubscriptionEvent{
		SubscriberID:     subscriberId,
		BucketID:         "b2",
		SubscriptionType: protos.SubscriptionType_BUCKET,
	}
	result2, err := client.Subscribe(context.Background(), object2)
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result2.Code)

	if errCon := conn.Close(); errCon != nil {
		logger.ErrorLogger.Println(errCon)
	}
}

func (e *Executor) SubscriberStream() {
	conn, err := e.mdsConnections["127.0.0.1"].Get(context.Background())
	if conn == nil || err != nil {
		logger.ErrorLogger.Println(err)
	}
	client := protos.NewMetadataServiceClient(conn.ClientConn)
	streamer, err := client.SubscribeStream(context.Background(), &protos.SubscriptionStreamEvent{
		SubscriberID: subscriberId,
	})
	logger.InfoLogger.Println("subscribing stream")
	for {
		object, err := streamer.Recv()
		if err == io.EOF {
			break
		}
		if err != nil {
			logger.ErrorLogger.Println(err)
			break
		}
		logger.InfoLogger.Println(object)
	}
	logger.InfoLogger.Println("got disconnected")
	if errCon := conn.Close(); errCon != nil {
		logger.ErrorLogger.Println(errCon)
	}
}

func (e *Executor) Unsubscribe() {
	conn, err := e.mdsConnections["127.0.0.1"].Get(context.Background())
	if conn == nil || err != nil {
		logger.ErrorLogger.Println(err)
	}
	client := protos.NewMetadataServiceClient(conn.ClientConn)
	object := &protos.SubscriptionEvent{
		SubscriberID:     subscriberId,
		Key:              "file3",
		BucketID:         "b2",
		SubscriptionType: protos.SubscriptionType_OBJECT,
	}
	result, err := client.Unsubscribe(context.Background(), object)
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result.Code)

	object2 := &protos.SubscriptionEvent{
		SubscriberID:     subscriberId,
		BucketID:         "b2",
		SubscriptionType: protos.SubscriptionType_BUCKET,
	}
	result2, err := client.Unsubscribe(context.Background(), object2)
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result2.Code)

	if errCon := conn.Close(); errCon != nil {
		logger.ErrorLogger.Println(errCon)
	}
}

func (e *Executor) ListObjects2() {
	conn, err := e.mdsConnections["127.0.0.1"].Get(context.Background())
	if conn == nil || err != nil {
		logger.ErrorLogger.Println(err)
	}
	client := protos.NewMetadataServiceClient(conn.ClientConn)

	result, err := client.List(context.Background(), &protos.ObjectListRequest{
		Prefix: &protos.ObjectID{
			Bucket: "b2",
			Key:    "path1/path2/",
		},
	})
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result)

	var del int32
	del = 47

	result, err = client.List(context.Background(), &protos.ObjectListRequest{
		Prefix: &protos.ObjectID{
			Bucket: "b2",
		},
		Delimiter: &del,
	})
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result)

	result, err = client.List(context.Background(), &protos.ObjectListRequest{
		Prefix: &protos.ObjectID{
			Bucket: "b2",
			Key:    "path1/path2/",
		},
		Delimiter: &del,
	})
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result)

	if errCon := conn.Close(); errCon != nil {
		logger.ErrorLogger.Println(errCon)
	}
}

func (e *Executor) ListObjects3() {
	conn, err := e.mdsConnections["127.0.0.1"].Get(context.Background())
	if conn == nil || err != nil {
		logger.ErrorLogger.Println(err)
	}
	client := protos.NewMetadataServiceClient(conn.ClientConn)

	result, err := client.List(context.Background(), &protos.ObjectListRequest{
		Prefix: &protos.ObjectID{
			Bucket: "b2",
			Key:    "photos/2006/",
		},
	})
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result)

	var del int32
	del = 47

	result, err = client.List(context.Background(), &protos.ObjectListRequest{
		Prefix: &protos.ObjectID{
			Bucket: "b2",
		},
		Delimiter: &del,
	})
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result)

	result, err = client.List(context.Background(), &protos.ObjectListRequest{
		Prefix: &protos.ObjectID{
			Bucket: "b2",
			Key:    "photos/2006/",
		},
		Delimiter: &del,
	})
	if err != nil {
		logger.ErrorLogger.Println(err)
	}
	logger.InfoLogger.Println(result)

	if errCon := conn.Close(); errCon != nil {
		logger.ErrorLogger.Println(errCon)
	}
}
