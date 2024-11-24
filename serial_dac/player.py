#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# wget https://ia802306.us.archive.org/20/items/NeverGonnaGiveYouUp/jocofullinterview41.mp3
# ffmpeg -i jocofullinterview41.mp3 -ac 1 -f f32le -ar 8000 rick.raw

# Check with: aplay -r 8000 -f FLOAT_LE < rick.raw

import time
import serial
import numpy as np

def main():
    dev = '/dev/ttyUSB0'
    rate = 115200

    song = np.fromfile('rick.raw', dtype=np.float32)
    song *= 0.3
    print(np.min(song))
    print(np.max(song))
    pos = 0
    ts = None

    with serial.Serial(dev, rate) as ser:
        while True:
            length = ser.read()[0]
            if length == 0xff:
                continue

            length *= 8

            data = song[pos:pos+length]

            # 8-bit:
            #data = (data * 127.5 + 127.5).astype(np.uint8)

            # 12-bit:
            data = (data * 2047.5 + 2047.5).astype(np.uint16)

            ser.write(data)
            #print('.', end='', flush=True)
            if not ts:
                ts = time.time()
                print('0')
            else:
                print(time.time() - ts)
                ts = time.time()

            pos += length

if __name__ == "__main__":
    main()
