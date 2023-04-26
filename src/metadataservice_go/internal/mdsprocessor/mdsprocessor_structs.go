/**
 * Copyright 2023- Technical University of Munich. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

package mdsprocessor

import (
	"github.com/IBM/gedsmds/internal/keyvaluestore"
	"github.com/IBM/gedsmds/internal/prommetrics"
	"github.com/IBM/gedsmds/internal/pubsub"
)

type Service struct {
	pubsub  *pubsub.Service
	kvStore *keyvaluestore.Service
	metrics *prommetrics.Metrics
}
