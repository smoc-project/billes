import time
import struct
import serial
import sys
from itertools import cycle

DEBUG_TYPE = 250
RADIO_FRAME_LEN = 13

def send_led_command(ser, led):
    ser.write(bytes([DEBUG_TYPE, led] + 8 * [42]))


with serial.Serial(sys.argv[1], 115200, timeout=1) as ser:
    for char in cycle([b'o', b'r', b'g', b'b']):
        print(char)
        send_led_command(ser, char[0])

        raw = ser.read(RADIO_FRAME_LEN)
        if raw:
            client_id = int(raw[0])

            x, y, z = struct.unpack_from("fff", raw[1:])
            print(f'Received: id:{client_id}, x:{x}, y:{y}, z:{z}')

        time.sleep(0.2)
