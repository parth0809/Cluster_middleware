# Cluster Middleware

## Overview

This project implements a small cluster middleware with four runtime roles:

- `client/client_app`: reads a job config and submits work
- `master/master_app`: primary scheduler
- `master/backup_app`: standby scheduler with failover support
- `compute/compute_app`: worker node that executes jobs

The system uses:

- TCP for job submission and forwarding
- UDP for compute heartbeats and result notifications
- SQLite for persistent logs

## Build

Build script: [build.sh](/home/parth/Downloads/sdfsafd/Clust%20(2)/Cluster_middlewae-main/kilpo/build.sh)

### Build outputs

| Component | Output |
|---|---|
| Client | `client/client_app` |
| Primary master | `master/master_app` |
| Backup master | `master/backup_app` |
| Compute node | `compute/compute_app` |

### Compiler settings

- `g++`
- `-std=c++14`
- `-pthread`
- `-lsqlite3` for master, backup, and compute

### Build command

```bash
./build.sh
```

## Run Order

Start the processes in this order:

1. `cd compute && ./compute_app`
2. `cd master && ./backup_app`
3. `cd master && ./master_app`
4. `cd client && ./client_app`

## Configuration Files

### Client system config

File: [client/client_config.json](/home/parth/Downloads/sdfsafd/Clust%20(2)/Cluster_middlewae-main/kilpo/client/client_config.json)

| Key | Meaning |
|---|---|
| `master_address` | Primary master address |
| `master_tcp_port` | Primary master TCP port |
| `backup_address` | Backup master address |
| `backup_tcp_port` | Backup master TCP port |
| `client_callback_address` | Callback address for failover recovery |

### Client job config

File: [client/config/config.json](/home/parth/Downloads/sdfsafd/Clust%20(2)/Cluster_middlewae-main/kilpo/client/config/config.json)

| Key | Meaning |
|---|---|
| `name` | Job name |
| `executable` | Executable path |
| `priority` | Job priority |
| `time_required` | Requested time metadata |
| `min_memory` | Minimum memory in MB |
| `min_cores` | Minimum CPU cores |
| `max_memory` | Maximum memory metadata |
| `gpu_required` | Optional GPU requirement |

### Primary master config

File: [master/master_config.json](/home/parth/Downloads/sdfsafd/Clust%20(2)/Cluster_middlewae-main/kilpo/master/master_config.json)

| Key | Meaning |
|---|---|
| `tcp_address` | TCP bind address |
| `tcp_port` | TCP listen port |
| `udp_address` | UDP bind address |
| `udp_port` | UDP heartbeat/result port |
| `backup_address` | Backup address |
| `backup_tcp_port` | Backup TCP port |
| `backup_connect_timeout_ms` | Replication timeout |

### Backup master config

File: [master/backup_config.json](/home/parth/Downloads/sdfsafd/Clust%20(2)/Cluster_middlewae-main/kilpo/master/backup_config.json)

| Key | Meaning |
|---|---|
| `tcp_address` | Backup TCP bind address |
| `tcp_port` | Backup TCP listen port |
| `udp_address` | Backup UDP bind address |
| `udp_port` | Backup UDP port |
| `primary_address` | Primary master address |
| `primary_tcp_port` | Primary master TCP port |
| `primary_check_interval_ms` | Health-check interval |
| `primary_check_timeout_ms` | Health-check timeout |
| `primary_failure_threshold` | Failures before promotion |
| `primary_startup_grace_ms` | Startup grace window |

### Compute config

File: [compute/compute_config.json](/home/parth/Downloads/sdfsafd/Clust%20(2)/Cluster_middlewae-main/kilpo/compute/compute_config.json)

| Key | Meaning |
|---|---|
| `compute_address` | Compute bind address |
| `compute_tcp_port` | Compute TCP port |
| `master_address` | Primary master UDP target |
| `master_udp_port` | Primary master UDP port |
| `backup_address` | Backup master UDP target |
| `backup_udp_port` | Backup master UDP port |
| `compute_available_resources` | Abstract capacity score |
| `compute_free_cores` | Free cores |
| `compute_free_memory_mb` | Free memory in MB |
| `compute_free_gpus` | Free GPUs |

## Typical Usage

1. Build with `./build.sh`.
2. Start compute, backup, and primary.
3. Start client.
4. At the client prompt, provide a job config such as `client/config/config.json`.

## Notes

- Client submits the executable itself by embedding it as base64 in the job payload.
- Compute nodes report heartbeats to both primary and backup.
- Backup can take over if the primary becomes unreachable.
- Detailed architecture and data design are documented in [designdoc.md](/home/parth/Downloads/sdfsafd/Clust%20(2)/Cluster_middlewae-main/kilpo/designdoc.md).
