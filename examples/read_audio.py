#!/usr/bin/env python3

import sys
import pymm

reader = pymm.TFFmpegReader(sys.argv[1])
print(len(reader))
print(reader.info(0))
slen = 0
while True:
    data = reader.read(0, 512)
    print('got {} {}'.format(len(data), len(data[0])))
    slen += len(data[0])
    if len(data[0]) == 0:
        break
print(slen)
