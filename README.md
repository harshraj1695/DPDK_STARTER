# DPDK Operation Example

This project demonstrates core DPDK (Data Plane Development Kit) operations using a clean, modular C code structure. It is designed to guide developers from basic EAL initialization to intermediate/advanced user-space packet processing.

The application shows how to perform high-speed networking using the Poll Mode Driver (PMD) model without requiring expensive physical NICs, making it perfect for learning in virtualized environments.

---

## ✨ Features

- **Zero-Copy Packet I/O**: Direct memory access for maximum throughput.
- **Poll-Mode Driver (PMD)**: Demonstrates constant polling instead of interrupt-driven processing.
- **TUN/TAP Support**: Uses virtual devices (`vdev`), allowing the code to run on **WSL2, Hyper-V, VirtualBox, and VMware**.
- **Modular C Structure**: Code is split into logical modules using `extern` declarations for professional organization.
- **Echo Server Logic**: Packets received are parsed, printed, and transmitted back.


---



### System Requirements

- **Linux**: Ubuntu 20.04/22.04+ recommended
- **Root Access**: Required for memory and device management
- **TUN/TAP Support**: Ensure `/dev/net/tun` exists
- **Build Tools**: `build-essential`, `pkg-config`, `meson`, `ninja`

---

## 📦 DPDK Installation

### Option 1: Via Package Manager

```bash
sudo apt update
sudo apt install libdpdk-dev build-essential pkg-config linux-headers-$(uname -r)
```

### Option 2: Build From Source

```bash
meson setup build
ninja -C build
sudo ninja -C build install
sudo ldconfig
```

**Verify Installation:**

```bash
pkg-config --modversion libdpdk
```

---

## 🚀 Building and Setup

### 1. Build the Project

```bash
make
```

### 2. Configure Hugepages (Recommended)

```bash
# Allocate 1024 pages of 2MB
echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

# Mount the hugepage filesystem
sudo mkdir -p /mnt/huge
sudo mount -t hugetlbfs nodev /mnt/huge
```

### 3. Create a Virtual Interface (TUN)

```bash
sudo ip tuntap add dev tun0 mode tun user $USER
sudo ip addr add 10.0.0.1/24 dev tun0
sudo ip link set tun0 up
```

---

## ▶️ Running the Application

```bash
sudo ./dpdk_tun_app -l 0-1 --no-huge --vdev=net_tap0,iface=tun0,mode=tun
```

### Expected Output

```plaintext
EAL initialized.
Ports available: 1
Mempool created.
Port 0 initialized and started.
Received X packets
Packet 0: len=...
```

---

## 🧪 Testing

### Ping Test

```bash
ping 10.0.0.1
```

### Manual Packet Injection (Scapy)

```python
from scapy.all import *
send(IP(dst="10.0.0.1")/ICMP(), iface="tun0")
```

### Monitoring

```bash
sudo tcpdump -i tun0 -vv
```

---

## 📜 Technical Notes

### Port & Queue Lifecycle

- **EAL Init**: `rte_eal_init()` parses args and discovers devices
- **Mempool**: `rte_pktmbuf_pool_create()` allocates buffers
- **Port Config**: `rte_eth_dev_configure()` sets queue counts
- **Queue Setup**: `rte_eth_rx_queue_setup()` binds mempool
- **Start**: `rte_eth_dev_start()` enables device
- **Processing**: `rte_eth_rx_burst()` fetches packets (zero-copy)

---

## 🧹 Cleanup

```bash
make clean
```
