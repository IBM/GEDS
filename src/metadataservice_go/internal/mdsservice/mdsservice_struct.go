package mdsservice

import (
	"github.com/IBM/gedsmds/internal/mdsprocessor"
	"github.com/IBM/gedsmds/internal/prommetrics"
)

type Service struct {
	processor *mdsprocessor.Service
	metrics   *prommetrics.Metrics
}
