import serial
import time

s = serial.Serial("/dev/ttyUSB0", baudrate=115200)
last = time.monotonic()
while True:
    b = s.read(1)
    if len(b) == 0:
        continue
    print("{:02X}@{:f}".format(b[0], (time.monotonic() - last)))
    last = time.monotonic();
