package prommetrics

import "github.com/prometheus/client_golang/prometheus"

type Metrics struct {
	testCounter prometheus.Counter
}
