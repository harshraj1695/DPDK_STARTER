L2 Forwarding with ACL Example
==============================

This example demonstrates use of the DPDK ACL library
to classify packets based on:

- protocol
- source IPv4
- destination IPv4
- source port
- destination port

Packets matching ACL rules are forwarded,
others are dropped.

Run:

./dpdk-l2fwd-acl -l 0-2 -n 4 -- -p 0x1
