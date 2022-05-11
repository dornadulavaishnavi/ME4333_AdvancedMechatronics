import board
import time
import digitalio
import analogio
from ulab import numpy as np # to get access to ulab numpy functions

sum_sin = np.zeros((1024,1))

for i in range(1024):
    sum_sin[i] = np.sin(i)+np.sin(6*i)+np.sin(4*i)

print("Hello World!")
