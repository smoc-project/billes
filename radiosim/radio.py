import time
import struct
import serial
import sys
import random
from itertools import cycle

LED_MSG = 250
POS_MSG = 251
RADIO_FRAME_LEN = 13

def send_led_command(ser, led):
    raw = bytes([LED_MSG, led] + 11 * [42])
    assert len(raw) == RADIO_FRAME_LEN
    ser.write(raw)

def send_pos_command(ser, x, y, z):
    raw = bytes([POS_MSG, *struct.pack("fff", x, y, z)])
    assert len(raw) == RADIO_FRAME_LEN
    ser.write(raw)

x ,y, z = 0, 0, 0
with serial.Serial(sys.argv[1], 115200, timeout=1) as ser:
    for i, char in enumerate(cycle([b'o', b'r', b'g', b'b'])):
        send_led_command(ser, char[0])

        raw = ser.read(RADIO_FRAME_LEN)
        if raw:
            client_id = int(raw[0])

            x, y, z = struct.unpack_from("fff", raw[1:])
            print(f'Received: id:{client_id}, x:{x}, y:{y}, z:{z}')

        time.sleep(0.1)

        if i % 4 == 0:
            x += random.random()
            y += random.random()
            z += random.random()
            send_pos_command(ser, x, y, z)
            print(f"x={x};y={y};z={z}")
