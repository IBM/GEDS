# GEDS Daemon Mode Redesign.

GEDS shall be redesigned to support multiple local instances. A central daemon is responsible
for managing files and sharing files between other daemons.
GEDS clients write and read to a mounted CSI hostpath which shares the data between pods.
NVMeoF and GPU Direct RDMA should be supported in a second step.

Goal:
- Enable data-sharing of GEDS objects between multiple Python processes.
- Enable data-sharing of GEDS objects across Kubernetes pods using a CSI hostpath.
- Data should survive crashes.
- Facilitate encryption.
- Enable reading/writing data to/from GPU and/or NVMe directly
- Native integration with Kubernetes


## Overview

The diagram shows a high-level overview.
```
 ┌────────────────────────────────────────────────────────────────────────────┐
 │ Kubernetes Node                                                            │
 │                                ┌────────────────────────────────────────┐  │
 │                                │ Kubernetes Pod                         │  │
 │                     IPC        │   ┌─────────────┐ ┌─────────────────┐  │  │
 │                    ┌───────────┼───► GEDS Client │ │ Apache Spark    │  │  │
 │                    │           │   └─────────────┘ └─────────────────┘  │  │
 │                    │           │                                        │  │
 │  ┌─────────────────┼────┐      └────────────────────────────────────────┘  │
 │  │ Kubernetes Pod  │    │                                                  │
 │  │  ┌──────────────▼──┐ │      ┌────────────────────────────────────────┐  │
 │  │  │ GEDS Daemon     ◄─┼──┐   │ Kubernetes Pod                         │  │
 │  │  └─────────────────┘ │  │   │   ┌─────────────┐ ┌──────────────────┐ │  │
 │  │                      │  ├───┼───► GEDS Client │ │ Python + Pytorch │ │  │
 │  │  ┌─────────────────┐ │  │   │   └─────────────┘ └──────────────────┘ │  │
 │  │  │ PVC / Local SSD │ │  │   │                                        │  │
 │  │  └─────────────────┘ │  │   │   ┌─────────────┐ ┌──────────────────┐ │  │
 │  │                      │  └───┼───► GEDS Client │ │ Python + Pytorch │ │  │
 │  └────────────────▲─────┘      │   └─────────────┘ └──────────────────┘ │  │
 │                   │            │                                        │  │
 │                   │            └────────────────────────────────────────┘  │
 │                   │ TCP/IP                                                 │
 │                   │                                                        │
 └───────────────────┼────────────────────────────────────────────────────────┘
                     │
 ┌───────────────────┼────────────────────────────────────────────────────────┐
 │ Kubernetes Node   │                                                        │
 │                   │                                                        │
 │  ┌────────────────▼─────┐      ┌────────────────────────────────────┐      │
 │  │ Kubernetes Pod       │      │ Kubernetes Pod                     │      │
 │  │  ┌─────────────────┐ │      │  ┌───────────────────────────────┐ │      │
 │  │  │ GEDS Daemon     │ │      │  │ GEDS Client                   │ │      │
 │  │  └─────────────────┘ │      │  └─────▲────────────────▲────▲───┘ │      │
 │  │                      │      │        │ I/O + MMAP     │    │     │      │
 │  │  ┌─────────────────┐ │Mounts│  ┌─────▼──────────┐     │    │     │      │
 │  │  │ PVC / Local SSD ◄─┼──────┼──┤  Hostpath CSI  │     │    │     │      │
 │  │  └─────────────────┘ │      │  └────────────────┘     │    │     │      │
 │  │                      │      │                         │    │     │      │
 │  └──────────────────────┘      └─────────────────────────┼────┼─────┘      │
 │                                                          │    │            │
 │                                               GPU Direct │    │NVMeOF      │
 │                                ┌─────────────────────────▼┐   │            │
 │                                │  GPU                     │   │            │
 │                                └───────────────────▲──────┘   │            │
 │                                                    │          │            │
 └────────────────────────────────────────────────────┼──────────┼────────────┘
                                                      │          │
                                      GPU Direct RDMA │          │
 ┌────────────────────────────────────────────────────▼──────────▼────────────┐
 │ NVMe JBOD                                                                  │
 │                                                                            │
 │                                                                            │
 │                                                                            │
 │                                                                            │
 │                                                                            │
 └────────────────────────────────────────────────────────────────────────────┘
```

## Operations

### Creating a file

1. GEDS Creates file with `BUCKET/PATH` and `UUID`.
2. GEDS Client creates a temporary file in the hostpath volume.
3. GEDS Client writes to the file.
4. GEDS Client seals the file. This transfers the ownership of the file to the
   GEDS daemon. FD is kept open and the daemon does a rename.
5. GEDS Daemon tells the metadata server that a new file has been
   created. 
   
   Conflict resolution:
    1. If no file with `BUCKET/PATH` does exist, the entry is created.
    2. If `BUCKET/PATH` already exists, then the existing entry is overridden.

6. GEDS Client reopens the file `Read-Only` and opens a file-descriptor.
7. Other GEDS Clients are now able to open this file.

### Opening a file

1. GEDS Client tells the daemon that it wants to open `FILE`.
2. GEDS Daemon:
    1. Checks with Metadata Server if the `BUCKET/PATH` exists. Receives `UUID` and locations.
    2. Increases the usage-count count of the filehandle to prevent deletion.
    3. Tells the `PATH` and size of `FILE` to the `Client`.

### Reading from a file

A file can be on one of the following locations:

- **Local**, if the file was created by a client on the same node, or if it is cached.
- **DFS**, if the file was stored on a distributed file system.
- **S3**, if the file is stored in a bucket on a S3-like service.
- **Remote**, if the file is stored on a different GEDS Daemon.

The GEDS Client will forward all reads to the GEDS Dameon for **Remote** files. All other file-types can be read directly by the GEDS Client.


### Closing a file

1. GEDS Client tells the daemon that it closed the `BUCKET/PATH`.
2. GEDS Daemon:
    1. Decreases the file-counter.
    2. If the file-counter is zero and the file is marked as deleted, delete the file.

### Deleting a file

1. GEDS Client tells the daemon that it wants to delete `BUCKET/PATH`.
2. GEDS Client closes local FD.
2. GEDS Daemon:
    1. Decreases reference counter for open `UUID`.
    2. Tells the metadata server that the path `BUCKET/PATH` has been deleted.
    3. Marks file as deleted. Delete the file if the reference counter is zero.

### Garbage collection

1. A daemon process automatically checks local file-usage.
2. If the defined quota is reached no files can be created.
3. Local copies can be deleted if:
    1. The file is marked as deleted.
    2. The file is not open and other copies exist (UUID matches).
    3. A previous version of the file is not referenced any more.

## Technical Implementations

- The TCP/IP transport is a GEDS Client as well. This way the transport requires no extra
  file/open/close mechanisms.
