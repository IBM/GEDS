package prommetrics

import (
	"github.com/IBM/gedsmds/internal/config"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promauto"
)

const promNamespace = "geds_mds_go"

func InitMetrics(registry prometheus.Registerer) *Metrics {
	metrics := &Metrics{}
	metrics.createObjectCounter = promauto.NewCounter(prometheus.CounterOpts{
		Namespace: promNamespace,
		Name:      "mds_create_objects_total",
		Help:      "The total number of create object calls",
	})
	metrics.lookupObjectCounter = promauto.NewCounter(prometheus.CounterOpts{
		Namespace: promNamespace,
		Name:      "mds_lookup_objects_total",
		Help:      "The total number of lookup object calls",
	})
	metrics.updateObjectCounter = promauto.NewCounter(prometheus.CounterOpts{
		Namespace: promNamespace,
		Name:      "mds_update_objects_total",
		Help:      "The total number of update object calls",
	})
	metrics.deleteObjectCounter = promauto.NewCounter(prometheus.CounterOpts{
		Namespace: promNamespace,
		Name:      "mds_delete_objects_total",
		Help:      "The total number of delete object calls",
	})
	metrics.listObjectCounter = promauto.NewCounter(prometheus.CounterOpts{
		Namespace: promNamespace,
		Name:      "mds_list_objects_total",
		Help:      "The total number of list object calls",
	})
	registry.MustRegister(metrics.createObjectCounter, metrics.lookupObjectCounter,
		metrics.updateObjectCounter, metrics.deleteObjectCounter, metrics.listObjectCounter)
	return metrics
}

func (m *Metrics) IncrementCreateObject() {
	if config.Config.PrometheusEnabled {
		m.createObjectCounter.Inc()
	}
}

func (m *Metrics) IncrementLookupObject() {
	if config.Config.PrometheusEnabled {
		m.lookupObjectCounter.Inc()
	}
}

func (m *Metrics) IncrementUpdateObject() {
	if config.Config.PrometheusEnabled {
		m.updateObjectCounter.Inc()
	}
}

func (m *Metrics) IncrementDeleteObject() {
	if config.Config.PrometheusEnabled {
		m.deleteObjectCounter.Inc()
	}
}

func (m *Metrics) IncrementListObject() {
	if config.Config.PrometheusEnabled {
		m.listObjectCounter.Inc()
	}
}
