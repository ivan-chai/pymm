#pragma once

#include <fstream>
#include <list>

struct AVFormatContext;
struct AVOutputFormat;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;


class TFFmpegAudioReader {
public:
    TFFmpegAudioReader(const char *fname);
    ~TFFmpegAudioReader();

    int SampleRate();
    int SampleWidth();
    int Channels();
    int Size();
    
    int Read(char *buf, int maxlen);

    void Close();

private:
    int ReadPacket();
    int ReadFrame();

private:
    AVFormatContext *avFormat;
    AVCodecContext *avCodec;

    AVPacket *packet;
    size_t packet_offs;
    AVFrame *frame;
    size_t frame_offs;
};

class TFFmpegAudioWriter {
public:
    TFFmpegAudioWriter(const char *fname, int sampleRate=16000, int sampleWidth=2, int channels=1);
    ~TFFmpegAudioWriter();

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
};
