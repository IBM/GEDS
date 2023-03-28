package prommetrics

import "github.com/prometheus/client_golang/prometheus"

type Metrics struct {
	createObjectCounter prometheus.Counter
	lookupObjectCounter prometheus.Counter
	updateObjectCounter prometheus.Counter
	deleteObjectCounter prometheus.Counter
	listObjectCounter   prometheus.Counter
}
