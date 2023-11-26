import time
import os

dev = "/dev/ledmatrix0"

assert os.path.exists(dev) == True

with open(dev, "wb") as f:
    state = False
    while True:
        f.raw.write(bytes([state]))
        time.sleep(1)
        state = not state