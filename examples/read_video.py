#!/usr/bin/env python3
"""Read video stream and show frames using matplotlib"""

import sys

import numpy as np
import matplotlib.pyplot as plt

import pymm


reader = pymm.TFFmpegReader(sys.argv[1])  # Open file
print(len(reader))  # Print number of streams
vids = None
for i in range(len(reader)):  # Find video stream and get description
    info = reader.info(i)
    print('Stream {}'.format(i))
    print(info)
    if info['type'] == pymm.EFF_VIDEO_STREAM:
        vids = i
        vidinfo = info
        width = info['width']
        height = info['height']
assert vids is not None, 'Couldn\'t find video stream'
slen = 0
while True:
    data = reader.read(vids, 5)  # Try to read 5 frames
    nsamps = len(data[0]) / vidinfo['sample_size']  # Number of frames returned
    if nsamps:
        print('got {} {}'.format(len(data), len(data[0])))
        image = np.frombuffer(data[0], dtype='u1').reshape((nsamps, height, width, 3))  # Convert buffer to numpy image
        print(image[0].base.shape)
        print(width, height)
        print(len(data[0])/nsamps, len(data[0])/(nsamps *width*height))
        print(image[0].shape)
        print(image[0].dtype)
        plt.imshow(image[0], aspect=1/vidinfo['aspect'])
        plt.show()
    slen += len(data[0])
    if len(data[0]) == 0:
        break
print(slen)
print(vidinfo['sample_size'])
print(slen / vidinfo['sample_size'])
