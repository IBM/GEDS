package keyvaluestore

import (
	"github.com/IBM/gedsmds/protos"
	"strings"
)

func (kv *Service) getNestedPath(objectId *protos.ObjectID) []string {
	nestedPath := strings.Split(strings.TrimSpace(objectId.Key), commonDelimiter)
	lenNestedPath := len(nestedPath)
	if lenNestedPath > 0 && len(nestedPath[lenNestedPath-1]) == 0 {
		nestedPath = nestedPath[:lenNestedPath-1]
	}
	return nestedPath
}
