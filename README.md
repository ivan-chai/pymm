# pymm
Simple Python3 ffmpeg interface for audio and video I/O.

This is development version. By now only audio/video decoding from file is supported

### Example

Read audio from file

```python

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


```

For video reading example see examples/read_video.py