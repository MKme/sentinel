"""Generic classifier handoff; no camera, model or aircraft-specific software.

Use with the Meshtastic Python client on your companion computer.
The node must run Sentinel with local observations and private mesh output enabled.
"""
import argparse
import secrets
import struct

def encode(label, confidence, count=1, observation_id=None, age_ms=0):
    label = label.upper().encode('ascii')
    if not 1 <= len(label) <= 24 or any(c not in b'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-' for c in label):
        raise ValueError('Label must be 1..24 letters, digits, underscores or hyphens')
    if not 0 <= confidence <= 100 or not 1 <= count <= 32767 or not 0 <= age_ms <= 5000:
        raise ValueError('Confidence 0..100, count 1..32767, age 0..5000 ms required')
    observation_id = observation_id if observation_id is not None else secrets.randbelow(0xffffffff) + 1
    if not 1 <= observation_id <= 0xffffffff:
        raise ValueError('Observation ID must be a nonzero unsigned 32-bit integer')
    return struct.pack('<4sIHHHB', b'SOI1', observation_id, round(confidence * 100), count, age_ms, len(label)) + label

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--port', required=True)
    p.add_argument('--label', required=True)
    p.add_argument('--confidence', type=float, required=True)
    p.add_argument('--count', type=int, default=1)
    a = p.parse_args()
    from meshtastic.serial_interface import SerialInterface
    interface = SerialInterface(devPath=a.port)
    try:
        payload = encode(a.label, a.confidence, a.count)
        interface.sendData(payload, destinationId=interface.myInfo.my_node_num, portNum=256, wantAck=False, wantResponse=False)
        print('Submitted locally; USB submission is not confirmation of mesh delivery.')
    finally:
        interface.close()
