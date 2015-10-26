#!/usr/bin/env python3

import numpy as np
import matplotlib.pyplot as plt

import pymm

import sys

reader = pymm.TFFmpegReader(sys.argv[1])
print(len(reader))
vids = None
for i in range(len(reader)):
    info = reader.info(i)
    print('Stream {}'.format(i))
    print(info)
    if info['type'] == pymm.EFF_VIDEO_STREAM:
        vids = i
        vidinfo = info
        width = info['width']
        height = info['height']
assert vids is not None, 'No video stream'
slen = 0
while True:
    data = reader.read(vids, 5)
    nsamps = len(data[0]) / vidinfo['sample_size']
    if nsamps:
        print('got {} {}'.format(len(data), len(data[0])))
        print(width, height)
        print(len(data[0])/nsamps, len(data[0])/(nsamps *width*height))
        image = np.frombuffer(data[0], dtype='u1').reshape((nsamps, height, width, 3))
        plt.imshow(image[0], aspect=1/vidinfo['aspect'])
        plt.show()
    slen += len(data[0])
    if len(data[0]) == 0:
        break
print(slen)
print(vidinfo['sample_size'])
print(slen / vidinfo['sample_size'])
