/**
 * Copyright 2023- Pezhman Nasirifard. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

package prommetrics

import "github.com/prometheus/client_golang/prometheus"

type Metrics struct {
	createObjectCounter          prometheus.Counter
	lookupObjectCounter          prometheus.Counter
	updateObjectCounter          prometheus.Counter
	deleteObjectCounter          prometheus.Counter
	listObjectCounter            prometheus.Counter
	pubSubMatchingCounter        prometheus.Counter
	pubSubSendPublicationCounter prometheus.Counter
}
