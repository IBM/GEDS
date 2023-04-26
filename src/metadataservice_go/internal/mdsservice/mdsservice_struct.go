/**
 * Copyright 2023- Technical University of Munich. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

package mdsservice

import (
	"github.com/IBM/gedsmds/internal/mdsprocessor"
	"github.com/IBM/gedsmds/internal/prommetrics"
)

type Service struct {
	processor *mdsprocessor.Service
	metrics   *prommetrics.Metrics
}
