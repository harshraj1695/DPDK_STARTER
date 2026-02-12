# DPDK TAP (TUN/TAP) Interface Setup and Usage Guide

## Install and Configure TAP Interface

### Create TAP Interface (Multi-Queue, User Access)

``` bash
sudo ip tuntap add dev tap0 mode tap user $USER multi_queue
```

### Alternative TAP Creation (Without User Parameter)

``` bash
sudo ip tuntap add dev tap0 mode tap multi_queue
```

### Bring TAP Interface Up

``` bash
sudo ip link set tap0 up
```

------------------------------------------------------------------------

## Assign IP Address to TAP Interface

``` bash
sudo ip addr add 10.0.0.2/24 dev tap0
```

------------------------------------------------------------------------

## Run DPDK Application with TAP Interface

``` bash
sudo ./sample -l 0 --no-huge --vdev=net_tap0,iface=tap0
```

------------------------------------------------------------------------

## Run DPDK Application with TAP Debug Logging

``` bash
sudo ./pipeline -l 0-3 --no-huge --no-pci   --vdev=net_tap0,iface=tap0   --log-level=pmd.net.tap:8
```
