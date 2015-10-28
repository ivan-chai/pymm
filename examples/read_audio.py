#!/usr/bin/env python3

import sys
import pymm

reader = pymm.TFFmpegReader(sys.argv[1]) #  Open file
print(len(reader)) #  Print number of streams
print(reader.info(0)) #  Print description of the first stream
slen = 0
while True:
    data = reader.read(0, 512) #  Read 512 audio samples from first stream
    print('got {} {}'.format(len(data), len(data[0]))) #  Print number of channels and data size in bytes
    slen += len(data[0])
    if len(data[0]) == 0: #  Nothing to read
        break
print(slen)
