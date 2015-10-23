%module pymm
%include "cstring.i"
%include "typemaps.i"
%{
#include "pymm.h"
%}

class TFFmpegAudioReader {
public:
    TFFmpegAudioReader(const char *fname);
    ~TFFmpegAudioReader();

    %rename(read) Read;
    %typemap(in) (char *buf, int maxlen) {
      $2 = PyLong_AsLong($input);
      if ($2 < 0) {
	SWIG_exception_fail(SWIG_TypeError, "Wrong data size");
      }
      $1 = new char[$2];
    }
    %typemap(out) int {
      $result = PyByteArray_FromStringAndSize(arg2, $1);
    }
    int Read(char *buf, int maxlen);
    %typemap(out) int {
      $result = PyLong_FromLong($1);
    }
    
    %rename(close) Close;
    void Close();

    %rename(samplerate) SampleRate;
    int SampleRate();
    %rename(samplewidth) SampleWidth;
    int SampleWidth();
    %rename(channels) Channels;
    int Channels();
    %rename(size) Size;
    int Size();
};

class TFFmpegAudioWriter {
public:
    TFFmpegAudioWriter(const char *fname, int sampleRate=16000, int sampleWidth=2, int channels=1);
    ~TFFmpegAudioWriter();

    %rename(write) Write;
    %typemap(in) (char *buf, size_t len) {
      if (!PyByteArray_Check($input)) {
	SWIG_exception_fail(SWIG_TypeError, "Need bytearray");
      }
      $1 = (char*) PyByteArray_AsString($input);
      $2 = (size_t) PyByteArray_Size($input);
    }
    int Write(char *buf, size_t len);

    %rename(flush) Flush;
    void Flush();

    %rename(close) Close;
    void Close();
};
