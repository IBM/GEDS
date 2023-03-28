# GEDS - A Generic Ephemeral Data Store

GEDS is a distributed epehemeral datastore that enables flexible scaling of compute and storage. It uses a centralized namenode to store active objects and transparently integrates into S3-like object-stores.

- [Building](doc/BUILDING.md)
- [geds_smart_open Python package](src/python/geds_smart_open/README.md)

## Ports

GEDS uses the following default ports (see also `src/utility/Ports.h`).
- **Web / Prometheus**: 4380
- **Metadata Server**: 4381
- **GEDS RPC**: 4382

## Benchmarking

1. Start the metadata server on `$HOST`:
   ```
   $GEDS_INSTALL/bin/metadataserver
   ```
2. Start the benchmarking server on a separate node
   ```
   GEDS_METADATASERVER_HOST=$HOST
   GEDS_METADATASERVER_PORT=4381
   export GEDS_METADATASERVER=${GEDS_METADATASERVER_HOST}:${GEDS_METADATASERVER_PORT}
   cd $GEDS_INSTALL/python
   python3.9 serve_benchmark.py
   ```
3. Run the benchmark command:
   ```bash
   GEDS_METADATASERVER_HOST=$HOST
   GEDS_METADATASERVER_PORT=4381
   $GEDS_INSTALL/bin/benchmark_io --address $GEDS_METADATASERVER_HOST --outputFile output.csv
   ```

### Contributing

Contributing to this repository requires a signed contributor license agreement (see [GEDS_CLA_Corporate](GEDS_CLA_Corporate.doc) for corporate copyright holders or [GEDS_CLA_Individual](GEDS_CLA_Individual.doc)).

All commits need to contain signed-off message to indicate that the submitter accepts the [DCO](DCO1.1.txt). Example
```
Commit message

Signed-off-by: John Doe <john.doe@example.com>
```
The command `git commit -s` will automatically add the message to the end of the commit.

This project welcomes external contributions. To contribute code or documentation, please submit a [pull request](https://github.com/IBM/GEDS/pulls).
