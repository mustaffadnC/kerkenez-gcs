"""Record a raw MAVLink byte stream from a running SITL into a test fixture.

Connects to SITL's TCP serial0, identifies itself as a GCS, requests all
telemetry streams at 4 Hz and dumps the raw received bytes unmodified.

Usage: py tools/record_stream.py [seconds] [outfile]
Defaults: 10 seconds -> tests/data/sitl_stream.bin
"""
import pathlib
import socket
import sys
import time

from pymavlink.dialects.v20 import common as mavlink2


def main() -> int:
    seconds = float(sys.argv[1]) if len(sys.argv) > 1 else 10.0
    default_out = pathlib.Path(__file__).resolve().parent.parent / "tests" / "data" / "sitl_stream.bin"
    outfile = pathlib.Path(sys.argv[2]) if len(sys.argv) > 2 else default_out

    sock = socket.create_connection(("127.0.0.1", 5760), timeout=5)
    mav = mavlink2.MAVLink(None, srcSystem=255, srcComponent=190)

    heartbeat = mav.heartbeat_encode(
        mavlink2.MAV_TYPE_GCS, mavlink2.MAV_AUTOPILOT_INVALID, 0, 0, mavlink2.MAV_STATE_ACTIVE
    )
    stream_req = mav.request_data_stream_encode(1, 0, mavlink2.MAV_DATA_STREAM_ALL, 4, 1)
    sock.send(heartbeat.pack(mav))
    sock.send(stream_req.pack(mav))

    chunks: list[bytes] = []
    deadline = time.time() + seconds
    sock.settimeout(1.0)
    last_heartbeat = time.time()
    while time.time() < deadline:
        try:
            data = sock.recv(4096)
        except socket.timeout:
            continue
        if not data:
            break
        chunks.append(data)
        if time.time() - last_heartbeat >= 1.0:
            sock.send(heartbeat.pack(mav))
            last_heartbeat = time.time()

    blob = b"".join(chunks)
    outfile.parent.mkdir(parents=True, exist_ok=True)
    outfile.write_bytes(blob)
    print(f"wrote {len(blob)} bytes to {outfile}")
    return 0 if blob else 1


if __name__ == "__main__":
    sys.exit(main())
