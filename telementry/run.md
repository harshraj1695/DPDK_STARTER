# Telemetry apps: how to build & run

This directory has **two** DPDK telemetry examples:

- `sample/` → minimal custom telemetry command: **`/harsh_stats`**
- `advanced_stats/` → richer telemetry commands: **`/adv/summary`**, **`/adv/lcore_stats`**

Both expose commands via the DPDK telemetry v2 socket (default):

- Socket: `/var/run/dpdk/rte/dpdk_telemetry.v2`
- Client protocol: **UNIX `SOCK_SEQPACKET`**, send command **without newline**

---

## 1) Minimal telemetry app (`sample/`)

### Build

```bash
cd /home/harshraj1695/dpdk/telementry/sample
make clean
make
```

### Run (server)

```bash
cd /home/harshraj1695/dpdk/telementry/sample
sudo ./main -l 0-2
```

### Query (client)

Using the provided Python client:

```bash
cd /home/harshraj1695/dpdk/telementry/sample
sudo python3 telementry.py "/harsh_stats"
```

List all commands (built-in + custom):

```bash
sudo python3 telementry.py "/"
```

Using the official interactive client (if installed):

```bash
sudo /usr/local/bin/dpdk-telemetry.py
```

Then type:

- `/` (list commands)
- `/harsh_stats`

---

## 2) Advanced telemetry app (`advanced_stats/`)

### Build

```bash
cd /home/harshraj1695/dpdk/telementry/advanced_stats
make clean
make
```

### Run (server)

```bash
cd /home/harshraj1695/dpdk/telementry/advanced_stats
sudo ./advanced_stats_main -l 0-2
```

### Query (client)

Summary stats (dict):

```bash
cd /home/harshraj1695/dpdk/telementry/advanced_stats
sudo python3 adv_client.py "/adv/summary"
```

Per-lcore stats (array):

```bash
sudo python3 adv_client.py "/adv/lcore_stats"
```

List all commands:

```bash
sudo python3 adv_client.py "/"
```

---

## Notes / common issues

- **Only one telemetry socket per file-prefix**: if you run both apps at the same time with the default file-prefix (`rte`), they will compete for the same telemetry socket name. If you want to run both simultaneously, start one of them with a different prefix, e.g.:

```bash
sudo ./main -l 0-2 --file-prefix=tele1
sudo ./advanced_stats_main -l 0-2 --file-prefix=tele2
```

Then the sockets will be:

- `/var/run/dpdk/tele1/dpdk_telemetry.v2`
- `/var/run/dpdk/tele2/dpdk_telemetry.v2`

- If the runtime dir is missing:

```bash
sudo mkdir -p /var/run/dpdk/rte
```

