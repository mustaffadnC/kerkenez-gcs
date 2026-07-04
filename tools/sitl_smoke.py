"""Smoke test: connect to a running ArduPilot SITL on TCP 5760 and wait for HEARTBEAT.

Usage: py tools/sitl_smoke.py [tcp:127.0.0.1:5760]
Exit code 0 on heartbeat within 15 s, 1 otherwise.
"""
import sys
import time

from pymavlink import mavutil


def main() -> int:
    endpoint = sys.argv[1] if len(sys.argv) > 1 else "tcp:127.0.0.1:5760"
    print(f"Connecting to {endpoint} ...")
    conn = mavutil.mavlink_connection(endpoint)

    deadline = time.time() + 15
    while time.time() < deadline:
        msg = conn.recv_match(type="HEARTBEAT", blocking=True, timeout=3)
        if msg is not None:
            print(
                f"HEARTBEAT ok: sysid={conn.target_system} compid={conn.target_component} "
                f"type={msg.type} autopilot={msg.autopilot} base_mode={msg.base_mode}"
            )
            return 0
    print("FAILED: no HEARTBEAT within 15 s")
    return 1


if __name__ == "__main__":
    sys.exit(main())
