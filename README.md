# pymm
Simple Python3 ffmpeg interface for audio and video I/O.

This is development version. By now only audio/video reading from file or from device is supported.

### Example

Read audio from file:

```python

import sys
import pymm

reader = pymm.TFFmpegReader(sys.argv[1])
print(len(reader))  # Number of streams in file
print(reader.info(0))  # Info for the first stream
slen = 0
while True:
    data = reader.read(0, 512)  # read 512 samples from first stream
    print('got {} {}'.format(len(data), len(data[0])))
    slen += len(data[0])
    if len(data[0]) == 0:
        break
print(slen)  # Total number of samples


```

For video reading example see examples/read_video.py

You can also capture webcam by reading /dev/video0 (Linux)