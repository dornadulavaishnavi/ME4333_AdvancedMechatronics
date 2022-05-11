import board
import time
import digitalio
import analogio
from ulab import numpy as np # to get access to ulab numpy functions
#from matplotlib import pyplot as plt

sum_sin = np.zeros((1024,1))
fft_sin = np.zeros((1024,1))

for i in range(1024):
    sum_sin[i] = np.sin(i)+np.sin(6*i)+np.sin(4*i)
    fft_sin[i] = np.fft.fft(sum_sin[i])
    print((fft_sin[i,0],))
    time.sleep(0.1)

#print("Hello World!")
