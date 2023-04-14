package main

import (
	"github.com/IBM/gedsmds/internal/mockgedsclient"
	"time"
	//_ "google.golang.org/grpc/encoding/gzip"
)

func main() {
	ex := mockgedsclient.NewExecutor()

	ex.ListBuckets()
	ex.ListObjectStore()
	ex.ListObjects()

	ex.RegisterObjectStore()
	ex.ListObjectStore()

	ex.CreateBucket()
	ex.ListBuckets()
	ex.LookUpBucket()
	//ex.DeleteBucket()
	ex.ListBuckets()
	ex.LookUpBucket()

	//ex.Lookup()
	ex.CreateObject()
	ex.ListObjects()
	ex.UpdateObject()
	ex.ListObjects()
	ex.DeleteObject()
	ex.Lookup()
	ex.DeletePrefix()
	//ex.ListObjects()

	ex.Subscribe()
	time.Sleep(2 * time.Second)
	go ex.SubscriberStream()
	time.Sleep(2 * time.Second)
	ex.Unsubscribe()
	time.Sleep(2 * time.Second)
	ex.ListObjects2()
}
