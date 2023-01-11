# GEDS smart_open package

## Environment variables

GEDS can be configured using the following environment variables:
- `GEDS_METADATASERVER`: GEDS Metadata Server, default: `geds-metadataserver:50001`.
- `GEDS_TMP`: GEDS Folder for ephemeral data, default: `/tmp/GEDS_XXXXXX`.
- `GEDS_BLOCK_SIZE`: GEDS Block size in bytes (optional).

## Reading and writing a file. 

```
from smart_open import open
from geds_smart_open import GEDS

lines = ['a\n', 'b\n']
with open("geds://python-test/test.txt", 'w') as f:
    for line in lines:
        f.write(line)


with open("geds://python-test/test.txt", 'r') as f:
    for line in f.readlines():
        print(line)
```

## Configuring an object store

Variant 1: Explicitly adding an object store:
```
import geds_smart_open

geds_smart_open.register_object_store(BUCKET, HOST, ACCESS_KEY, SECRET_KEY)
```

Variant 2: Injecting the credentials into the `geds` path:
```
with open("geds://ACCESS_KEY:SECRET_KEY@HOST:443@BUCKET/KEY", 'r') as f:
    for line in f.readlines():
        print(line)
```
- Port 443 indicates HTTPS
- Port 80 indicates HTTP
