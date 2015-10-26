#pragma once

#include <exception>

class TFFmpegException {
    const char *Msg;
public:
    TFFmpegException(const char *msg) {Msg = msg;}
    const char *GetMessage() {return Msg;}
};

enum TFFmpegStreamType {
    EFF_UNK_STREAM,
    EFF_AUDIO_STREAM,
    EFF_VIDEO_STREAM
};

struct TFFmpegSampleType {
    bool Int;
    bool Signed;
};

struct TFFmpegStreamInfo {
    TFFmpegStreamType Type;
    TFFmpegSampleType SampleType;
    float SampleRate;
    int SampleSize;
    //Audio only
    int Channels;
    //Video only
    int Width;
    int Height;
    float Ratio;
};

class TFFmpegReaderImp;
class TFFmpegWriterImp;

class TFFmpegReader {
public:
    TFFmpegReader(const char* fname);
    ~TFFmpegReader();

    int StreamNum();
    TFFmpegStreamInfo StreamInfo(int stream);

    int Size(int stream);
    int Read(int stream, int sampNum, char*** data, int* channels);
    
    void Close();

private:
    TFFmpegReaderImp *Imp;
};

/*class TFFmpegWriter {
public:
    TFFmpegWriter(const char *fname, int sampleRate=16000, int sampleWidth=2, int channels=1);
    ~TFFmpegWriter();

    int Write(char *buf, size_t len);

    void Close();
    void Flush();
private:
    int WritePacket();
    int WriteFrame();

    static int write(void *opaque, unsigned char *buf, int buf_size);

private:
    bool header_writed;
    int SampleWidth;
    int Channels;
    size_t frame_len;
    size_t frame_offs;
    unsigned char *iobuf;
    AVFormatContext *avFormat;
    AVCodecContext *avCodec;

    AVPacket *packet;
    AVFrame *frame;
    };*/
