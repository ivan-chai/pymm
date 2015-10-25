%module pymm
%include "cstring.i"
%include "typemaps.i"
%{
#include "pymm.h"
%}

enum TFFmpegStreamType {
  EFF_UNK_STREAM,
  EFF_AUDIO_STREAM,
  EFF_VIDEO_STREAM
};
   
struct TFFmpegStreamInfo {
  TFFmpegStreamType Type;
  int SampleRate;
  int SampleSize;
  //Audio only
  int Channels;
  //Video only
  int Width;
  int Height;
};

// char** data
%typemap(in, numinputs=0) char **data (char *tmpptr) {
  $1 = &tmpptr;
}
%typemap(argout) char** data {
  Py_XDECREF($result);
  $result = PyByteArray_FromStringAndSize(*$1, result);
}

// stream info
%typemap(out) TFFmpegStreamInfo {
  $result = PyDict_New();
  PyDict_SetItemString($result, "type", PyLong_FromLong($1.Type));
  PyDict_SetItemString($result, "sample_rate", PyLong_FromLong($1.SampleRate));
  PyDict_SetItemString($result, "sample_size", PyLong_FromLong($1.SampleSize));
  if($1.Type == EFF_AUDIO_STREAM) {
    PyDict_SetItemString($result, "channels", PyLong_FromLong($1.Channels));
  } else if($1.Type == EFF_VIDEO_STREAM) {
    PyDict_SetItemString($result, "width", PyLong_FromLong($1.Width));
    PyDict_SetItemString($result, "height", PyLong_FromLong($1.Height));
  }
}

class TFFmpegReader {
public:
    TFFmpegReader(const char *fname);
    ~TFFmpegReader();

    %rename(read) Read;
    int Read(int stream, int sampNum, char** data);
    
    %rename(close) Close;
    void Close();
    
    %rename(__len__) StreamNum;
    int StreamNum();
    
    %rename(info) StreamInfo;
    TFFmpegStreamInfo StreamInfo(int stream);
    
    %rename(size) Size;
    int Size(int stream);
};

/*class TFFmpegWriter {
public:
    TFFmpegWriter(const char *fname, int sampleRate=16000, int sampleWidth=2, int channels=1);
    ~TFFmpegWriter();

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
*/
