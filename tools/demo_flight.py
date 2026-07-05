"""Scripted demo flight for a running Copter SITL.

Owns SITL's serial0 (TCP 5760): connecting there also unblocks SITL's boot,
which only then opens the serial1/serial2 listeners (5762/5763) — point the
GCS at 5762. Waits until arming is possible, then:
GUIDED -> arm -> takeoff 40 m -> two guided waypoints -> RTL.

Usage: py tools/demo_flight.py [endpoint]   (default tcp:127.0.0.1:5760)
"""
import sys
import time

from pymavlink import mavutil

HOME = (39.925533, 32.866287)
TAKEOFF_ALT = 40.0
# position-only type_mask for SET_POSITION_TARGET_GLOBAL_INT
POSITION_ONLY = 0x0DF8


def log(text: str) -> None:
    print(text, flush=True)


def wait_ack(conn, command: int, timeout: float = 5.0):
    deadline = time.time() + timeout
    while time.time() < deadline:
        msg = conn.recv_match(type="COMMAND_ACK", blocking=True, timeout=1)
        if msg is not None and msg.command == command:
            return msg.result
    return None


def connect_with_retry(endpoint: str, attempts: int = 15):
    for attempt in range(attempts):
        try:
            return mavutil.mavlink_connection(endpoint)
        except ConnectionRefusedError:
            log(f"waiting for {endpoint} (attempt {attempt + 1})")
            time.sleep(2)
    raise SystemExit(f"could not connect to {endpoint}")


def main() -> int:
    endpoint = sys.argv[1] if len(sys.argv) > 1 else "tcp:127.0.0.1:5760"
    conn = connect_with_retry(endpoint)
    conn.wait_heartbeat(timeout=60)
    log(f"HEARTBEAT sysid={conn.target_system}")

    # GPS_RAW_INT / GLOBAL_POSITION_INT are streamed messages — off by default.
    conn.mav.request_data_stream_send(conn.target_system, conn.target_component,
                                      mavutil.mavlink.MAV_DATA_STREAM_ALL, 4, 1)

    # Order matters. A GUIDED request during early boot is "accepted" but the
    # autopilot reverts to Stabilize when init finishes — so first wait for a
    # 3D fix (EKF origin follows), only then switch mode and confirm it stuck
    # via HEARTBEAT.custom_mode.
    log("waiting for 3D GPS fix")
    deadline = time.time() + 120
    while time.time() < deadline:
        msg = conn.recv_match(type="GPS_RAW_INT", blocking=True, timeout=2)
        if msg is not None and msg.fix_type >= 3:
            log("3D fix")
            break
    else:
        log("FAILED: no GPS fix")
        return 1

    mode_id = conn.mode_mapping()["GUIDED"]
    for attempt in range(60):
        conn.mav.set_mode_send(conn.target_system,
                               mavutil.mavlink.MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, mode_id)
        msg = conn.recv_match(type="HEARTBEAT", blocking=True, timeout=2)
        if msg is not None and msg.custom_mode == mode_id:
            log(f"mode GUIDED confirmed (attempt {attempt + 1})")
            break
        time.sleep(1)
    else:
        log("FAILED: GUIDED not accepted")
        return 1

    for attempt in range(40):
        conn.mav.command_long_send(conn.target_system, conn.target_component,
                                   mavutil.mavlink.MAV_CMD_COMPONENT_ARM_DISARM,
                                   0, 1, 0, 0, 0, 0, 0, 0)
        result = wait_ack(conn, mavutil.mavlink.MAV_CMD_COMPONENT_ARM_DISARM)
        if result == mavutil.mavlink.MAV_RESULT_ACCEPTED:
            log(f"ARMED (attempt {attempt + 1})")
            break
        time.sleep(3)
    else:
        log("FAILED: could not arm")
        return 1

    for attempt in range(10):
        conn.mav.command_long_send(conn.target_system, conn.target_component,
                                   mavutil.mavlink.MAV_CMD_NAV_TAKEOFF,
                                   0, 0, 0, 0, 0, 0, 0, TAKEOFF_ALT)
        result = wait_ack(conn, mavutil.mavlink.MAV_CMD_NAV_TAKEOFF)
        if result == mavutil.mavlink.MAV_RESULT_ACCEPTED:
            break
        log(f"takeoff not accepted (result={result}), retrying")
        time.sleep(2)
    else:
        log("FAILED: takeoff rejected")
        return 1
    log("TAKEOFF")

    def wait_alt(target: float, timeout: float = 90.0) -> None:
        deadline = time.time() + timeout
        while time.time() < deadline:
            msg = conn.recv_match(type="GLOBAL_POSITION_INT", blocking=True, timeout=2)
            if msg is not None and msg.relative_alt / 1000.0 >= target:
                return
        log(f"warning: altitude {target} m not reached in {timeout} s")

    wait_alt(TAKEOFF_ALT * 0.5)
    log("AIRBORNE")

    waypoints = [
        (HOME[0] + 0.0030, HOME[1], 45.0),
        (HOME[0] + 0.0015, HOME[1] + 0.0035, 35.0),
    ]
    for i, (lat, lon, alt) in enumerate(waypoints, 1):
        conn.mav.set_position_target_global_int_send(
            0, conn.target_system, conn.target_component,
            mavutil.mavlink.MAV_FRAME_GLOBAL_RELATIVE_ALT_INT, POSITION_ONLY,
            int(lat * 1e7), int(lon * 1e7), alt, 0, 0, 0, 0, 0, 0, 0, 0)
        log(f"GOTO {i}")
        time.sleep(18)

    conn.mav.command_long_send(conn.target_system, conn.target_component,
                               mavutil.mavlink.MAV_CMD_NAV_RETURN_TO_LAUNCH,
                               0, 0, 0, 0, 0, 0, 0, 0)
    log("RTL")
    time.sleep(10)
    log("DONE")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
