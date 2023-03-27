package prommetrics

import (
	"github.com/IBM/gedsmds/internal/logger"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promauto"
	"time"
)

const promNamespace = "geds_mds_go"

func InitMetrics(registry prometheus.Registerer) {
	metrics := &Metrics{}
	metrics.testCounter = promauto.NewCounter(prometheus.CounterOpts{
		Namespace: promNamespace,
		Name:      "processed_ops_total_mds",
		Help:      "The total number of processed events",
	})
	registry.MustRegister(metrics.testCounter)
	go metrics.runTestMetrics()
}

func (m *Metrics) runTestMetrics() {
	go func() {
		for {
			m.testCounter.Inc()
			time.Sleep(2 * time.Second)
			logger.InfoLogger.Println("countermetric++")
		}
	}()
}
