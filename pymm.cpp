#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <list>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

#include "pymm.h"

//=================================================================
//
// TFFmpegAudioReader
//
//=================================================================

class TFFmpegReaderImp {
private:
    struct TFFmpegStream {
	TFFmpegStreamType Type;
	AVCodecContext *CodecCtx;

	std::list<AVPacket> Cache;
	int SampleSize;
	
	AVFrame* Frame;
	AVPacket* Packet;
	int FrameSize;
	int FrameOffs;

	//Audio only
	TFFmpegSampleType SampleType;

	//Video only
	AVFrame* HelperFrame;
	uint8_t* HelperVideoBuf;
	SwsContext *SwsCtx;

	//Debug
	int PacketNum;
	int FrameNum;

	// METHODS
	TFFmpegStream() {
	    CodecCtx = NULL;
	    Frame = NULL;
	    Packet = NULL;
	    HelperFrame = NULL;
	    HelperVideoBuf = NULL;
	    SwsCtx = NULL;
	    PacketNum = 0;
	    FrameNum = 0;
	}
	~TFFmpegStream() {
	    avcodec_close(CodecCtx);
	    if (Frame != NULL)
		av_free(Frame);
	    for(auto i = Cache.begin(); i != Cache.end(); i++)
		av_free_packet(&(*i));
	    if(HelperVideoBuf)
		delete HelperVideoBuf;
	    if(HelperFrame) 
		av_free(HelperFrame);
	    if(SwsCtx) 
		av_free(SwsCtx);
	}
	void Init() {
	    if(Type == EFF_AUDIO_STREAM)
		InitAudio();
	    else if(Type == EFF_VIDEO_STREAM)
		InitVideo();
	    Frame = av_frame_alloc();
	    if(Frame == NULL)
		throw TFFmpegException("Couldn't allocate frame");
	    FrameSize = 0;
	    FrameOffs = 0;
	}
	void InitAudio() {
	    switch (CodecCtx->sample_fmt) {
	    case AV_SAMPLE_FMT_U8:
	    case AV_SAMPLE_FMT_U8P:
		SampleType.Int = true;
		SampleType.Signed = false;
		SampleSize = 1;
		break;
	    case AV_SAMPLE_FMT_S16:
	    case AV_SAMPLE_FMT_S16P:
		SampleType.Int = true;
		SampleType.Signed = true;
		SampleSize = 2;
		break;
	    case AV_SAMPLE_FMT_S32:
	    case AV_SAMPLE_FMT_S32P:
		SampleType.Int = true;
		SampleType.Signed = true;
		SampleSize = 4;
		break;
	    case AV_SAMPLE_FMT_FLT:
	    case AV_SAMPLE_FMT_FLTP:
		SampleType.Int = false;
		SampleType.Signed = true;
		SampleSize = sizeof(float);
		break;
	    case AV_SAMPLE_FMT_DBL:
	    case AV_SAMPLE_FMT_DBLP:
		SampleType.Int = false;
		SampleType.Signed = true;
		SampleSize = sizeof(double);
		break;
	    default:
		throw TFFmpegException("Could not detect sample size");
		
	    }
	}
	void InitVideo() {
	    SampleSize = avpicture_get_size(CodecCtx->pix_fmt, CodecCtx->width, CodecCtx->height);
	    int HelperBufSize = avpicture_get_size(PIX_FMT_RGB24, CodecCtx->width, CodecCtx->height);
	    HelperFrame = av_frame_alloc();
	    if(HelperFrame == NULL)
		throw TFFmpegException("Couldn't allocate frame");
	    HelperVideoBuf = new uint8_t[HelperBufSize];
	    avpicture_fill((AVPicture *)HelperFrame, HelperVideoBuf,
			   PIX_FMT_RGB24,
			   CodecCtx->width, CodecCtx->height);
	    SwsCtx = sws_getContext(CodecCtx->width,
				    CodecCtx->height,
				    CodecCtx->pix_fmt,
				    CodecCtx->width,
				    CodecCtx->height,
				    PIX_FMT_RGB24,
				    SWS_BILINEAR,
				    NULL,
				    NULL,
				    NULL
		);
	}
	int Read(int toRead, char** data, int sampNum) {
	    int (*DecodeMethod) (AVCodecContext *, AVFrame *, int *, const AVPacket *);
	    if(Type == EFF_AUDIO_STREAM)
		DecodeMethod = &avcodec_decode_audio4;
	    else if(Type == EFF_VIDEO_STREAM)
		DecodeMethod = &avcodec_decode_video2;
	    if(Type == EFF_UNK_STREAM)
		throw TFFmpegException("Wrong stream type");

	    int ToReadOrig = toRead;
	    int ToCopy;
	    int BytesN;
	    int GotFrame;
	    while(toRead > 0) {
		if (FrameSize > 0) {
		    //Append decoded data
		    ToCopy = FFMIN(FrameSize, toRead);
		    if(Type == EFF_AUDIO_STREAM) {
			for(int i = 0; i < CodecCtx->channels; ++i) {
			    memcpy(data[i] + SampleSize * (sampNum - toRead),
				   Frame->data[i] + SampleSize * FrameOffs,
				   SampleSize * ToCopy);
			}
		    } else if(Type == EFF_VIDEO_STREAM) {
			memcpy(data[0] + SampleSize * (sampNum - toRead),
			       Frame->data[0] + SampleSize * FrameOffs,
			       SampleSize * ToCopy);
		    }
		    toRead -= ToCopy;
		    FrameSize -= ToCopy;
		    FrameOffs += ToCopy;
		} else if (Packet) {
		    //Decode new portion
		    do {
			BytesN = DecodeMethod(CodecCtx, Frame, &GotFrame, Packet);
			if(BytesN < 0)
			    throw TFFmpegException("Decoding error");
			if(Packet->size) {
			    Packet->data += BytesN;
			    Packet->size -= BytesN;
			}
		    } while(GotFrame == 0 && BytesN > 0);
		    if(GotFrame) {
			FrameNum ++;
			FrameOffs = 0;
			if(Type == EFF_AUDIO_STREAM)
			    FrameSize = Frame->nb_samples;
			else if(Type == EFF_VIDEO_STREAM)
			    FrameSize = 1;
		    } else if (Packet->size == 0) {
			av_free_packet(Packet);
			Cache.pop_front();
			Packet = NULL;
		    }
		} else if(Cache.size()) {
		    //Get new packet
		    PacketNum ++;
		    Packet = &(*Cache.begin());
		} else {
		    //No data left
		    break;
		}
	    }
	    return ToReadOrig - toRead;
	}
    };
    
    AVFormatContext *FormatCtx;
    std::vector<TFFmpegStream> Streams;
    AVPacket* Packet;

public:
    TFFmpegReaderImp(const char* fname) {
	FormatCtx = NULL;
	Packet = NULL;
	
	av_log_set_level(AV_LOG_QUIET);
	av_register_all();
	if(avformat_open_input(&FormatCtx, fname, NULL, NULL) != 0)
	    throw TFFmpegException("Couldn't open file");
	if(avformat_find_stream_info(FormatCtx, NULL) < 0)
	    throw TFFmpegException("Stream info not found");

	// Init streams
	Streams.resize(FormatCtx->nb_streams);
	for(size_t i = 0; i < Streams.size(); ++i) {
	    TFFmpegStream& Stream = Streams[i];
	    AVCodecContext *CodecCtxTmp = FormatCtx->streams[i]->codec;
	    if(CodecCtxTmp->codec_type == AVMEDIA_TYPE_AUDIO)
		Stream.Type = EFF_AUDIO_STREAM;
	    else if(CodecCtxTmp->codec_type == AVMEDIA_TYPE_VIDEO) {
		Stream.Type = EFF_VIDEO_STREAM;
	    } else {
		Stream.Type = EFF_UNK_STREAM;
		continue;
	    }
	    AVCodec *Codec = avcodec_find_decoder(CodecCtxTmp->codec_id);
	    if(Codec == NULL)
		throw TFFmpegException("Codec not found");
	    Stream.CodecCtx = avcodec_alloc_context3(Codec);
	    if(avcodec_copy_context(Stream.CodecCtx, CodecCtxTmp) != 0) 
		throw TFFmpegException("Couldn't copy codec context");
	    if(avcodec_open2(Stream.CodecCtx, Codec, NULL) < 0)
		throw TFFmpegException("Couldn't open codec");
	    avcodec_close(CodecCtxTmp);

	    Stream.Init();
	}

	Packet = new AVPacket();
	av_init_packet(Packet);
    }

    ~TFFmpegReaderImp() {
	Close();
    }
    
    void Close() {
	Streams.clear();
	if (Packet != NULL) {
	    delete Packet;
	}
	if (FormatCtx) {
	    avformat_close_input(&FormatCtx);
	    FormatCtx = 0;
	}
    }
    
    int StreamNum() {
	return Streams.size();
    }

    void CheckStream(int stream) {
	if(stream < 0 || stream >= (int) Streams.size()) {
	    throw TFFmpegException("Stream not found");
	}
    }
    
    TFFmpegStreamInfo StreamInfo(int stream) {
	CheckStream(stream);
	
	TFFmpegStreamInfo info;
	TFFmpegStream& Stream = Streams[stream];
	info.Type = Stream.Type;
	info.SampleType = Stream.SampleType;
	info.SampleSize = Stream.SampleSize;
	if(Stream.Type == EFF_AUDIO_STREAM) {
	    info.SampleRate = Stream.CodecCtx->sample_rate;
	    info.Channels = Stream.CodecCtx->channels;
	} else if(Stream.Type == EFF_VIDEO_STREAM) {
	    info.SampleRate = Stream.CodecCtx->time_base.den / ((float) Stream.CodecCtx->time_base.num * Stream.CodecCtx->ticks_per_frame);
	    info.Width = Stream.CodecCtx->width;
	    info.Height = Stream.CodecCtx->height;
	} else {
	    throw TFFmpegException("Wrong stream type");
	}
	
	return info;
    }

    int Size(int stream) {
	throw TFFmpegException("Not implemented");
    }
    
    int Read(int stream, int sampNum, char*** data, int* channels) {
	CheckStream(stream);
	TFFmpegStream& Stream = Streams[stream];
	*channels = 1;
	if(Stream.Type == EFF_AUDIO_STREAM)
	    *channels = Stream.CodecCtx->channels;
	int BufSize = Stream.SampleSize * sampNum;
	*data = new char*[*channels];
	for(int i = 0; i < *channels; ++i)
	    (*data)[i] = new char[BufSize];

	int ToRead = sampNum;
	while(ToRead > 0) {
	    if(Stream.Cache.size()) {
		ToRead -= Stream.Read(ToRead, *data, sampNum);
	    } else {
		if(av_read_frame(FormatCtx, Packet) < 0)
		    break;
		if(Packet->buf == NULL) {
		    // Make copy for longer lifetime
		    AVPacket Dup;
		    av_copy_packet(&Dup, Packet);
		    av_free_packet(Packet);
		    *Packet = Dup;
		}
		Streams[Packet->stream_index].Cache.push_back(*Packet);
	    }
	}
	int PerChannelBytes = Stream.SampleSize * (sampNum - ToRead);
	return PerChannelBytes;
    }
    
    int Size() {
	//Not implemented
	return 0;
    }
};

TFFmpegReader::TFFmpegReader(const char* fname) {
  Imp = new TFFmpegReaderImp(fname);
}

TFFmpegReader::~TFFmpegReader() {
  delete Imp;
}

int TFFmpegReader::StreamNum() {
  return Imp->StreamNum();
}

TFFmpegStreamInfo TFFmpegReader::StreamInfo(int stream) {
  return Imp->StreamInfo(stream);
}

int TFFmpegReader::Size(int stream) {
  return Imp->Size(stream);
}

int TFFmpegReader::Read(int stream, int sampNum, char*** data, int* channels) {
    return Imp->Read(stream, sampNum, data, channels);
}

void TFFmpegReader::Close() {
  Imp->Close();
}

//=================================================================
//
// TFFmpegAudioWriter
//
//=================================================================
/*int TFFmpegWriter::write(void *opaque, uint8_t *buf, int buf_size)
{
    return 0;
}

TFFmpegWriter::TFFmpegWriter(const char *fname, int sampleRate, int sampleWidth, int channels) {
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

TFFmpegWriter::~TFFmpegWriter() {
    Close();
}

int TFFmpegWriter::Write(char *buf, size_t len) {
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

int TFFmpegWriter::WritePacket() {
    if (packet == NULL || !header_writed)
        return -1;

    av_write_frame(avFormat, packet);
    
    av_free_packet(packet);
    packet->data = NULL;
    packet->size = 0;
    av_init_packet(packet);

    return 0;
}

int TFFmpegWriter::WriteFrame() {
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

void TFFmpegWriter::Flush() {
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

void TFFmpegWriter::Close() {
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
*/
