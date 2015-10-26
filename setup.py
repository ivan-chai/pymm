#!/usr/bin/env python3

"""
Distutils/setuptools installer for pymm.
"""

import os
import subprocess
from distutils.core import setup, Extension

os.environ['CC'] = 'g++'

ffmpeg_wrapper = Extension('_pymm',
                    libraries = [ 'avformat', 'avcodec', 'avutil', 'avdevice', 'swscale' ], #?swresample
                    sources = [ 'pymm.i', 'pymm.cpp' ],
                    swig_opts=['-c++'],
                    extra_compile_args=['-std=c++11']
                    )

def make_binds():
    subprocess.call('swig -python -py3 -c++ -o pymm_wrap.cpp pymm.i'.split())

#make_binds()

setup(name='pymm',
      version='0.0.1',
      license='GPLv3',
      author='Ivan Karpukhin',
      url='https://github.com/ivan-chai/pymm',
      description='Audio and video I/O library',
      long_description='Simple Python3 wrapper for audio and video I/O using FFmpeg',
#      packages=['pymm'],
#      package_data={'pymm': ['_pymm.so', 'pymm.py', '__init__.py']},
      ext_modules = [ ffmpeg_wrapper ])
