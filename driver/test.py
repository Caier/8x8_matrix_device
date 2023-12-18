import time
import os

dev = "/dev/ledmatrix0"

assert os.path.exists(dev) == True

img = 0b1010101001010101101010100101010110101010010101011010101001010101

with open(dev, "wb") as f:
    f.raw.write(img.to_bytes(8))