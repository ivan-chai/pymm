#include <iostream>
#include <fstream>
#include <cstring>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

#include "pymm.h"

//=================================================================
//
// TFFmpegAudioReader
//
//=================================================================

TFFmpegAudioReader::TFFmpegAudioReader(const char *fname) {
    avFormat = NULL;
    avCodec = NULL;
    packet = NULL;

    avcodec_register_all();
    av_register_all();

    avformat_open_input(&avFormat, fname, NULL, NULL);
    avformat_find_stream_info(avFormat, NULL);
    int avStreamIdx = av_find_best_stream(avFormat, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    AVStream *avStream = avFormat->streams[avStreamIdx];
    avCodec = avStream->codec;
    
    AVCodec *decoder = avcodec_find_decoder(avCodec->codec_id);
    AVDictionary *opts = NULL;
    avcodec_open2(avCodec, decoder, &opts);

    packet = new AVPacket();
    av_init_packet(packet);
    packet->data = NULL;
    packet->size = 0;
    packet_offs = 0;
    
    frame = avcodec_alloc_frame();
    frame->nb_samples = 0;
    frame_offs = 0;
}

TFFmpegAudioReader::~TFFmpegAudioReader() {
    Close();
}

int TFFmpegAudioReader::Read(char *buf, int maxdata) {
    if (frame == NULL) {
        return 0;
    }

    int w = SampleWidth();

    size_t to_read = maxdata;
    while (to_read > 0) {
        size_t to_copy;
        size_t line_size = frame->nb_samples * w;
        if (line_size - frame_offs >= to_read) {
            to_copy = to_read;
        } else {
            to_copy = line_size - frame_offs;
        }
        if (to_copy > 0) {
            std::memcpy(buf + maxdata - to_read, frame->extended_data[0] + frame_offs, to_copy);
            frame_offs += to_copy;
        }
        to_read -= to_copy;
        if (frame_offs >= line_size) {
            if (ReadFrame() < 0)
                break;
        }
    }
    return maxdata - to_read;
}

int TFFmpegAudioReader::ReadPacket() {
    if (packet == NULL)
        return -1;

    av_free_packet(packet);
    if (av_read_frame(avFormat, packet) < 0) {
        packet->size = 0;
        packet_offs = 0;
        return -1;
    }
    packet_offs = 0;
    return 0;
}

int TFFmpegAudioReader::ReadFrame() {
    if (frame == NULL)
        return -1;

    int got_frame = 0;
    frame_offs = 0;
    while (got_frame == 0) {
        if (packet->size - packet_offs == 0) {
            if (ReadPacket() < 0) {
                frame->nb_samples = 0;
                return -1;
            }
        }
        AVPacket decPacket = *packet;
        decPacket.data += packet_offs;
        decPacket.size -= packet_offs;
        int decoded = avcodec_decode_audio4(avCodec, frame, &got_frame, &decPacket);
        decoded = FFMIN(decoded, packet->size);
        if (decoded < 0)
            break;
        packet_offs += decoded;
    }
    return 0;
}

void TFFmpegAudioReader::Close() {
    if (avCodec != NULL) {
        avcodec_close(avCodec);
        avCodec = NULL;
    }

    if (avFormat != NULL) {
        avformat_close_input(&avFormat);
        avFormat = NULL;
    }
    
    if (packet != NULL) {
        av_free_packet(packet);
        delete packet;
        packet = NULL;
    }
    
    if (frame != NULL) {
        av_free(frame);
        frame = NULL;
    }
}
int TFFmpegAudioReader::SampleRate() {
    if (avCodec == NULL)
        return -1;
    return avCodec->sample_rate;
}

int TFFmpegAudioReader::SampleWidth() {
    if (avCodec == NULL)
        return -1;
    switch (avCodec->sample_fmt) {
        case AV_SAMPLE_FMT_U8:
            return 1;
        case AV_SAMPLE_FMT_S16:
            return 2;
        case AV_SAMPLE_FMT_S32:
            return 4;
        default:
            return -1;
    }
}

int TFFmpegAudioReader::Channels() {
    if (avCodec == NULL)
        return -1;
    return avCodec->channels;
}

int TFFmpegAudioReader::Size() {
  //Not implemented
  return 0;
}

//=================================================================
//
// TFFmpegAudioWriter
//
//=================================================================

int TFFmpegAudioWriter::write(void *opaque, uint8_t *buf, int buf_size)
{
    return 0;
}

TFFmpegAudioWriter::TFFmpegAudioWriter(const char *fname, int sampleRate, int sampleWidth, int channels) {
    avFormat = NULL;
    avCodec = NULL;
    packet = NULL;
    frame = NULL;
    iobuf = NULL;
    frame_len = 0;
    frame_offs = 0;
    header_writed = false;
    Channels = channels;
    SampleWidth = sampleWidth;
   
    avcodec_register_all();
    av_register_all();

    AVOutputFormat *avOutFormat = av_guess_format(NULL, fname, NULL);
    if (!avOutFormat) {
        std::cerr << "Can't guess format" << std::endl;
        Close();
        return;
    }
    if (strcmp(avOutFormat->name, "ogg") == 0)
        avOutFormat->audio_codec = CODEC_ID_VORBIS;
    AVCodec *encoder = avcodec_find_encoder(avOutFormat->audio_codec);
    //TODO check supported formats
    /*if (encoder->supported_samplerates) {
        bool supported = false;
        std::cout << "SUPPORTED SAMPLE RATES:" << std::endl;
        for (size_t i = 0; encoder->supported_samplerates[i]; ++i) {
            std::cout << encoder->supported_samplerates[i] << ", ";
            if (encoder->supported_samplerates[i] == sampleRate)
                supported = true;
        }
        std::cout << std::endl;
        if (!supported) {
            std::cout << "WRONG SAMPLE RATE" << std::endl;
            return;
        }
    }*/
    avFormat = avformat_alloc_context();
    avFormat->oformat = avOutFormat;
    
    AVStream *stream = avformat_new_stream(avFormat, encoder);
    if (stream == NULL) {
        std::cerr << "Can't create stream" << std::endl;
        Close();
        return;
    }
    avCodec = stream->codec;
    avCodec->codec_type = AVMEDIA_TYPE_AUDIO;
    avCodec->bit_rate = 0;
    avCodec->sample_rate = sampleRate;
    avCodec->channels = channels;
    switch (sampleWidth) {
        case 1:
            avCodec->sample_fmt = AV_SAMPLE_FMT_U8;
            break;
        case 2:
            avCodec->sample_fmt = AV_SAMPLE_FMT_S16;
            break;
        case 4:AV_SAMPLE_FMT_S32:
            avCodec->sample_fmt = AV_SAMPLE_FMT_S32;
            break;
        default:
            avCodec->sample_fmt = AV_SAMPLE_FMT_S16;
    }

    AVDictionary *opts = NULL;
    //av_dict_set(&opts, "flags", "global_header", 0);
    int ret = avcodec_open2(avCodec, encoder, &opts);
    av_dict_free(&opts);
    opts = NULL;
    
    packet = new AVPacket();
    packet->data = NULL;
    packet->size = 0;
    av_init_packet(packet);
    
    frame = avcodec_alloc_frame();
    if (avCodec->frame_size == 0)
        avCodec->frame_size = 1024;
    av_samples_get_buffer_size(frame->linesize, channels, avCodec->frame_size, avCodec->sample_fmt, 4);
    frame_len = frame->linesize[0];
//    std::cout << "FRAME SIZE: " << frame->linesize[0] << std::endl;
    frame->extended_data = (uint8_t **)av_malloc(sizeof(uint8_t*) * Channels);
    for (size_t i = 0; i < Channels; ++i) {
        frame->data[i] = new uint8_t[frame_len];
        frame->extended_data[i] = new uint8_t[frame_len];
    }
    frame->nb_samples = 0;
   
    avio_open(&avFormat->pb, fname, AVIO_FLAG_READ_WRITE);
    ret = avformat_write_header(avFormat, NULL);
    if (ret != 0) {
        std::cerr << "Failed to write header" << std::endl;
        Close();
        return;
    }
    header_writed = true;
}

TFFmpegAudioWriter::~TFFmpegAudioWriter() {
    Close();
}

int TFFmpegAudioWriter::Write(char *buf, size_t len) {
    if (packet == NULL || frame == NULL) {
        return 0;
    }
    size_t buf_size = len;
    while (buf_size > 0) {
        size_t free_space = frame_len - frame_offs;
        size_t to_write;
        if (buf_size >= free_space)
            to_write = free_space;
        else
            to_write = buf_size;
        
        std::memcpy(frame->data[0] + frame_offs, buf, to_write);
        frame_offs += to_write;

        if (to_write == free_space) {
            frame->nb_samples = frame_len / SampleWidth;
            WriteFrame();
            frame_offs = 0;
        }

        buf += to_write;
        buf_size -= to_write;
    };

    return len;
}

int TFFmpegAudioWriter::WritePacket() {
    if (packet == NULL || !header_writed)
        return -1;

    av_write_frame(avFormat, packet);
    
    av_free_packet(packet);
    packet->data = NULL;
    packet->size = 0;
    av_init_packet(packet);

    return 0;
}

int TFFmpegAudioWriter::WriteFrame() {
    if (frame == NULL || packet == NULL || !header_writed)
        return -1;

    int got_packet = 0;
    //std::cout << "ENC: " << avcodec_encode_audio2(avCodec, packet, frame, &got_packet) << std::endl;
    int ret = avcodec_encode_audio2(avCodec, packet, frame, &got_packet);
    if (ret < 0) {
        std::cerr << "Can't encode audio" << std::endl;
        Close();
        return -1;
    }
    if (got_packet)
        WritePacket();
    return 0;
}

void TFFmpegAudioWriter::Flush() {
    if (!packet || !header_writed)
        return;

    if (frame_offs > 0) {
        frame->nb_samples = frame_offs / SampleWidth;
        WriteFrame();
        frame_offs = 0;
    }

    int got_packet = 0;
    avcodec_encode_audio2(avCodec, packet, NULL, &got_packet);
    if (got_packet)
        WritePacket();
}

void TFFmpegAudioWriter::Close() {
    if (header_writed) {
        Flush();
        av_write_trailer(avFormat);
        header_writed = false;
    }

    if (avCodec) {
        avcodec_close(avCodec);
        avCodec = NULL;
    }

    if (avFormat) {
        if (avFormat->pb) {
            avio_close(avFormat->pb);
            avFormat->pb = NULL;
        }
        avformat_free_context(avFormat);
        avFormat = NULL;
    }
    
    if (packet) {
        av_free_packet(packet);
        delete packet;
        packet = NULL;
    }
    
    if (frame) {
        for (size_t i = 0; i < Channels; ++i) {
            delete[] frame->data[i];
            delete[] frame->extended_data[i];
        }
        av_free(frame->extended_data);
        frame->extended_data = NULL;
        av_free(frame);
        frame = NULL;
    }
}
