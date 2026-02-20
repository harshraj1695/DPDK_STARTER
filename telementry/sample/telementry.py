#!/usr/bin/env python3
import socket
import sys

SOCKET_PATH = "/var/run/dpdk/rte/dpdk_telemetry.v2"

# Allow command as argument, default to "/" to list commands
cmd = sys.argv[1] if len(sys.argv) > 1 else "/"
print(f"Sending command: '{cmd}'")

# DPDK telemetry v2 uses SOCK_SEQPACKET (as per official script)
try:
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_SEQPACKET)
    sock.connect(SOCKET_PATH)
except Exception as e:
    print(f"Connection failed: {e}")
    sys.exit(1)

# Receive handshake (JSON with version info)
try:
    hello = sock.recv(4096)
    print("Handshake:", hello.decode())
except Exception as e:
    print(f"Handshake receive failed: {e}")
    sock.close()
    sys.exit(1)

# Send command - DPDK telemetry v2 does NOT use newline!
# The official dpdk-telemetry.py uses: sock.send(cmd.encode()) without newline
try:
    sent = sock.send(cmd.encode())
    print(f"Sent {sent} bytes: {repr(cmd)}")
except Exception as e:
    print(f"Send failed: {e}")
    sock.close()
    sys.exit(1)

# Receive response
try:
    resp = sock.recv(16384)
    print("Response:", resp.decode())
except Exception as e:
    print(f"Receive failed: {e}")

sock.close()
