# Control vs Data Packet Segregation

This DPDK application receives packets from one port, classifies each packet as **control-plane** or **data-plane**, and segregates them into two separate `rte_ring` queues.

## Control vs data

- **Control packets** (enqueued to `control_ring`):  
  ICMP, OSPF (IP proto 89), DNS/DHCP/NTP/SNMP (UDP 53/67/68/123/161), SSH/BGP/LDAP (TCP 22/179/389).

- **Data packets** (enqueued to `data_ring`):  
  All other IPv4 traffic (e.g. HTTP, HTTPS, custom apps).

Non-IPv4 frames are treated as data.

## Build

```bash
make
```

Requires DPDK (pkg-config `libdpdk` or installed under `/usr/local`).

## Run

### Without a physical NIC (af_packet)

Use the **af_packet** PMD with a Linux interface (e.g. a veth pair or tap). Create a veth pair or tap if needed, then:

```bash
./main -l 0 --vdev=net_af_packet0,iface=<interface>
```

Example with a veth interface named `veth0`:

```bash
./main -l 0 --vdev=net_af_packet0,iface=veth0
```

Replace `<interface>` with your Linux interface name (e.g. `veth0`, `tap0`, or the end of a veth pair you created). Traffic sent to that interface will be received by DPDK; packets the app sends back go out the same interface.

### With a physical NIC

Bind the port to DPDK (e.g. `dpdk-devbind.py`), then:

```bash
./main -l 0 -a <PCI_ADDR>
```

## Behavior

1. RX burst from the port.
2. For each packet: parse L2/L3/L4 and classify as control or data.
3. Enqueue to `control_ring` or `data_ring`.
4. Drain both rings and re-transmit on the same port (optional; can be changed to drop or send to different ports).
5. Print control/data packet counts periodically.

You can change the drain logic to send control and data to different ports or queues, or to different lcores, as needed.
