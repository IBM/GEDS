package keyvaluestore

import (
	"github.com/IBM/gedsmds/internal/keyvaluestore/db"
	"github.com/IBM/gedsmds/protos"
	"strings"
)

func (kv *Service) getNestedPath(objectID *protos.ObjectID) []string {
	nestedPath := strings.Split(strings.TrimSpace(objectID.Key), db.CommonDelimiter)
	lenNestedPath := len(nestedPath)
	if lenNestedPath > 0 && len(nestedPath[lenNestedPath-1]) == 0 {
		nestedPath = nestedPath[:lenNestedPath-1]
	}
	return nestedPath
}

func (kv *Service) filterIDsAndCommonPrefix(objectsIDs []string, objectQuery *protos.ObjectID) ([]string, []string) {
	filteredIDs := []string{}
	filteredPrefix := []string{}
	tempCommonPrefixes := map[string]bool{}
	for _, objectID := range objectsIDs {
		keyParts := strings.Split(objectID, db.CommonDelimiter)
		if len(objectQuery.Key) == 0 {
			if len(keyParts) == 2 {
				filteredIDs = append(filteredIDs, objectID)
			} else if len(keyParts) > 2 {
				tempCommonPrefixes[keyParts[2]] = true
			}
		} else {
			prefixLength := len(strings.Split(objectQuery.Key, db.CommonDelimiter)) + 2
			if len(keyParts) == prefixLength {
				tempCommonPrefixes[strings.Join(keyParts[:prefixLength-1], db.CommonDelimiter)] = true
			}
		}
	}
	if len(tempCommonPrefixes) > 0 {
		for commonPrefix := range tempCommonPrefixes {
			filteredPrefix = append(filteredPrefix, commonPrefix+db.CommonDelimiter)
		}
	}
	return filteredIDs, filteredPrefix
}
