# GEDS - A Generic Ephemeral Data Store

GEDS is a distributed epehemeral datastore that enables flexible scaling of compute and storage. It uses a centralized namenode to store active objects and transparently integrates into S3-like object-stores.

- [Building](doc/BUILDING.md)
- [geds_smart_open Python package](src/python/geds_smart_open/README.md)

## Benchmarking

1. Start the metadata server on `$HOST`:
   ```
   $GEDS_INSTALL/bin/metadataserver
   ```
2. Start the benchmarking server on a separate node
   ```
   GEDS_METADATASERVER_HOST=$HOST
   GEDS_METADATASERVER_PORT=50001
   export GEDS_METADATASERVER=${GEDS_METADATASERVER_HOST}:${GEDS_METADATASERVER_PORT}
   cd $GEDS_INSTALL/python
   python3.9 serve_benchmark.py
   ```
3. Run the benchmark command:
   ```bash
   GEDS_METADATASERVER_HOST=$HOST
   GEDS_METADATASERVER_PORT=50001
   $GEDS_INSTALL/bin/benchmark --serverAddress $GEDS_METADATASERVER_HOST --serverPort $GEDS_METADATASERVER_PORT --outputFile output.csv
   ```
