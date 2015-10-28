"""A setuptools based setup module.

See:
https://github.com/ivan-chai/pymm
"""

from distutils.core import setup, Extension

ffmpeg_wrapper = Extension('_pymm',
                           libraries = [ 'avformat', 'avcodec', 'avutil', 'avdevice', 'swscale' ],
                           sources = [ 'pymm.i', 'pymm.cpp' ],
                           swig_opts=['-c++'],
                           extra_compile_args=['-std=c++11']
                    )

setup(name='pymm',
      version='0.1.0a1',
      description='Audio and video I/O library',
      long_description='Simple Python3 wrapper for audio and video I/O using FFmpeg',
      url='https://github.com/ivan-chai/pymm',
      author='Ivan Karpukhin',
      author_email='karpuhini@yandex.ru',
      license='GPLv3+',
      classifiers=[
          'Development Status :: 3 - Alpha',

          'Intended Audience :: Developers',
          'Topic :: Software Development :: Libraries :: Python Modules',

          'License :: OSI Approved :: GNU General Public License v3 or later (GPLv3+)',

          'Programming Language :: Python :: 2',
          'Programming Language :: Python :: 2.6',
          'Programming Language :: Python :: 2.7',
          'Programming Language :: Python :: 3',
          'Programming Language :: Python :: 3.2',
          'Programming Language :: Python :: 3.3',
          'Programming Language :: Python :: 3.4'
          ],
      keywords='ffmpeg multimedia development',
      ext_modules = [ ffmpeg_wrapper ])
