# -*- coding: utf-8 -*-
"""
Created on Mon Aug 10 16:14:17 2020

@author: Seo
"""

# add the outside code routines
import sys
addedpaths = ["F:\\PycharmProjects\\pydsproutines"]
for path in addedpaths:
    if path not in sys.path:
        sys.path.append(path)
        
from xcorrRoutines import *
from filterRoutines import *
import numpy as np
import scipy as sp
import scipy.signal as sps
import matplotlib.pyplot as plt

cutout = np.fromfile("D:/SignalObserver/preamble.bin", np.complex64)

data = np.fromfile("D:/SignalObserver/rawfile1.bin", np.int16).astype(np.float32).view(np.complex64)

chnBW = 5e3
fs = 2e6
f_tap = sps.firwin(20000, chnBW/fs)
chnls = wola(f_tap, data, int(fs/chnBW), int(fs/chnBW * 2), dtype=np.complex64)

chnl = chnls[:,568].flatten()

productpeaks, freqlist_inds = fastXcorr(cutout, chnl, True, outputCAF=False, shifts=None, absResult=True)
plt.plot(productpeaks)



