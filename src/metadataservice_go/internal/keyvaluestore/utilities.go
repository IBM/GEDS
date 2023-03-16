package keyvaluestore

import (
	"github.com/IBM/gedsmds/protos/protos"
	"strings"
)

func (kv *Service) getNestedPath(objectId *protos.ObjectID) []string {
	return strings.Split(objectId.GetKey(), commonDelimiter)
}
