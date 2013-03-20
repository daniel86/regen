/*
 * audio-stream.cpp
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#include <AL/al.h>    // OpenAL header files
#include <AL/alc.h>
#include <AL/alext.h>

#ifndef AL_MONO_SOFT
#define AL_MONO_SOFT                             0x1500
#define AL_STEREO_SOFT                           0x1501
#define AL_REAR_SOFT                             0x1502
#define AL_QUAD_SOFT                             0x1503
#define AL_5POINT1_SOFT                          0x1504
#define AL_6POINT1_SOFT                          0x1505
#define AL_7POINT1_SOFT                          0x1506
#endif
#ifndef AL_BYTE_SOFT
#define AL_BYTE_SOFT                             0x1400
#define AL_UNSIGNED_BYTE_SOFT                    0x1401
#define AL_SHORT_SOFT                            0x1402
#define AL_UNSIGNED_SHORT_SOFT                   0x1403
#define AL_INT_SOFT                              0x1404
#define AL_UNSIGNED_INT_SOFT                     0x1405
#define AL_FLOAT_SOFT                            0x1406
#define AL_DOUBLE_SOFT                           0x1407
#endif

#include <ogle/utility/logging.h>
#include <ogle/utility/string-util.h>

#include "audio-stream.h"
using namespace ogle;

static ALenum avToAlType(AVSampleFormat format)
{
  switch(format)
  {
  case AV_SAMPLE_FMT_U8:   ///< unsigned 8 bits
    return AL_UNSIGNED_BYTE_SOFT;
  case AV_SAMPLE_FMT_S16:  ///< signed 16 bits
    return AL_SHORT_SOFT;
  case AV_SAMPLE_FMT_S32:  ///< signed 32 bits
    return AL_INT_SOFT;
  case AV_SAMPLE_FMT_FLT:  ///< float
    return AL_FLOAT_SOFT;
  case AV_SAMPLE_FMT_DBL:  ///< double
    return AL_DOUBLE_SOFT;
#if LIBAVCODEC_VERSION_MAJOR>53
  case AV_SAMPLE_FMT_U8P:  ///< unsigned 8 bits, planar
    return AL_UNSIGNED_BYTE_SOFT;
  case AV_SAMPLE_FMT_S16P: ///< signed 16 bits, planar
    return AL_SHORT_SOFT;
  case AV_SAMPLE_FMT_S32P: ///< signed 32 bits, planar
    return AL_INT_SOFT;
  case AV_SAMPLE_FMT_FLTP: ///< float, planar
    return AL_FLOAT_SOFT;
  case AV_SAMPLE_FMT_DBLP: ///< double, planar
    return AL_DOUBLE_SOFT;
#endif
  case AV_SAMPLE_FMT_NONE:
  default:
    throw new AudioStream::Error(FORMAT_STRING(
        "unsupported sample format " << format));
  }
}
static ALenum avToAlLayout(uint64_t layout)
{
  switch(layout)
  {
  case AV_CH_LAYOUT_MONO:
    return AL_MONO_SOFT;
  case AV_CH_LAYOUT_STEREO:
    return AL_STEREO_SOFT;
  case AV_CH_LAYOUT_QUAD:
    return AL_QUAD_SOFT;
  case AV_CH_LAYOUT_5POINT1:
    return AL_5POINT1_SOFT;
  case AV_CH_LAYOUT_7POINT1:
    return AL_7POINT1_SOFT;
  default:
    throw new AudioStream::Error(FORMAT_STRING(
        "unsupported channel layout " << layout));
  }
}
static ALenum avFormat(ALenum type, ALenum layout)
{
  switch(type)
  {
  case AL_UNSIGNED_BYTE_SOFT:
    switch(layout) {
    case AL_MONO_SOFT:    return AL_FORMAT_MONO8;
    case AL_STEREO_SOFT:  return AL_FORMAT_STEREO8;
    case AL_QUAD_SOFT:    return alGetEnumValue("AL_FORMAT_QUAD8");
    case AL_5POINT1_SOFT: return alGetEnumValue("AL_FORMAT_51CHN8");
    case AL_7POINT1_SOFT: return alGetEnumValue("AL_FORMAT_71CHN8");
    default: throw new AudioStream::Error("unsupported format");
    }
  case AL_SHORT_SOFT:
    switch(layout) {
    case AL_MONO_SOFT:    return AL_FORMAT_MONO16;
    case AL_STEREO_SOFT:  return AL_FORMAT_STEREO16;
    case AL_QUAD_SOFT:    return alGetEnumValue("AL_FORMAT_QUAD16");
    case AL_5POINT1_SOFT: return alGetEnumValue("AL_FORMAT_51CHN16");
    case AL_7POINT1_SOFT: return alGetEnumValue("AL_FORMAT_71CHN16");
    default: throw new AudioStream::Error("unsupported format");
    }
  case AL_FLOAT_SOFT:
    switch(layout) {
    case AL_MONO_SOFT:    return alGetEnumValue("AL_FORMAT_MONO_FLOAT32");
    case AL_STEREO_SOFT:  return alGetEnumValue("AL_FORMAT_STEREO_FLOAT32");
    case AL_QUAD_SOFT:    return alGetEnumValue("AL_FORMAT_QUAD32");
    case AL_5POINT1_SOFT: return alGetEnumValue("AL_FORMAT_51CHN32");
    case AL_7POINT1_SOFT: return alGetEnumValue("AL_FORMAT_71CHN32");
    default: throw new AudioStream::Error("unsupported format");
    }
  case AL_DOUBLE_SOFT:
    switch(layout) {
    case AL_MONO_SOFT:    return alGetEnumValue("AL_FORMAT_MONO_DOUBLE");
    case AL_STEREO_SOFT:  return alGetEnumValue("AL_FORMAT_STEREO_DOUBLE");
    default: throw new AudioStream::Error("unsupported format");
    }
  default: throw new AudioStream::Error("unsupported format");
  }
}

AudioStream::AudioStream(AVStream *stream, GLint index, GLuint chachedBytesLimit)
: AudioVideoStream(stream, index, chachedBytesLimit),
  audioSource_( ref_ptr<AudioSource>::manage( new AudioSource ) ),
  alType_( avToAlType(codecCtx_->sample_fmt) ),
  alChannelLayout_( avToAlLayout(codecCtx_->channel_layout) ),
  alFormat_( avFormat(alType_, alChannelLayout_) ),
  rate_( codecCtx_->sample_rate )
{
  DEBUG_LOG("init audio stream" <<
      " AL format=" << alFormat_ <<
      " sample_fmt=" << codecCtx_->sample_fmt <<
      " channel_layout=" << codecCtx_->channel_layout <<
      " sample_rate=" << codecCtx_->sample_rate <<
      " bit_rate=" << codecCtx_->bit_rate <<
      ".");
#if LIBAVCODEC_VERSION_MAJOR>53
  // create resample context for planar sample formats
  if (av_sample_fmt_is_planar(codecCtx_->sample_fmt)) {
    int out_sample_fmt;
    switch(codecCtx_->sample_fmt) {
    case AV_SAMPLE_FMT_U8P:  out_sample_fmt = AV_SAMPLE_FMT_U8; break;
    case AV_SAMPLE_FMT_S16P: out_sample_fmt = AV_SAMPLE_FMT_S16; break;
    case AV_SAMPLE_FMT_S32P: out_sample_fmt = AV_SAMPLE_FMT_S32; break;
    case AV_SAMPLE_FMT_DBLP: out_sample_fmt = AV_SAMPLE_FMT_DBL; break;
    case AV_SAMPLE_FMT_FLTP:
    default: out_sample_fmt = AV_SAMPLE_FMT_FLT;
    }

    resampleContext_ = avresample_alloc_context();
    av_opt_set_int(resampleContext_,
        "in_channel_layout",  codecCtx_->channel_layout, 0);
    av_opt_set_int(resampleContext_,
        "in_sample_fmt",      codecCtx_->sample_fmt,     0);
    av_opt_set_int(resampleContext_,
        "in_sample_rate",     codecCtx_->sample_rate,    0);
    av_opt_set_int(resampleContext_,
        "out_channel_layout", codecCtx_->channel_layout, 0);
    av_opt_set_int(resampleContext_,
        "out_sample_fmt",     out_sample_fmt,            0);
    av_opt_set_int(resampleContext_,
        "out_sample_rate",    codecCtx_->sample_rate,    0);
    avresample_open(resampleContext_);
    DEBUG_LOG("converting sample format to " << out_sample_fmt << ".");
  }
#endif
}
AudioStream::~AudioStream()
{
#if LIBAVCODEC_VERSION_MAJOR>53
  if(resampleContext_) avresample_free(&resampleContext_);
#endif
  clearQueue();
}

void AudioStream::clearQueue()
{
  alSourceStop(audioSource_->id());
  alSourcei(audioSource_->id(), AL_BUFFER, 0);

  while(decodedFrames_.size()>0) {
    AVFrame *f = frontFrame(); popFrame();
    AudioFrame *buf = (AudioFrame*)f->opaque;

    audioSource_->unqueue(*buf->buffer);
    buf->free();
    delete buf;
  }
}

void AudioStream::decode(AVPacket *packet)
{
  AVFrame *frame = avcodec_alloc_frame();
  // Decode audio frame
#if LIBAVCODEC_VERSION_MAJOR>53
  int frameFinished = 0;
  avcodec_decode_audio4(codecCtx_, frame, &frameFinished, packet);
  if(!frameFinished) {
    av_free(frame);
    return;
  }
#else
  codecCtx_->get_buffer(codecCtx_, frame);
  int bytesDecoded = AVCODEC_MAX_AUDIO_FRAME_SIZE;
  avcodec_decode_audio3(codecCtx_, (int16_t*)frame->data[0], &bytesDecoded, packet);
#endif

  // unqueue processed buffers
  ALint processed;
  alGetSourcei(audioSource_->id(), AL_BUFFERS_PROCESSED, &processed);
  for(; processed>0; --processed)
  {
    ALuint bufid;
    alSourceUnqueueBuffers(audioSource_->id(), 1, &bufid);
    AVFrame *processedFrame = frontFrame(); popFrame();

    AudioFrame *af = (AudioFrame*)processedFrame->opaque;
    af->free();
    delete af;
  }

  AudioFrame *audioFrame = new AudioFrame;
  audioFrame->avFrame = frame;
  audioFrame->buffer = new AudioBuffer;
  // Get the required buffer size for the given audio parameters.
#if LIBAVCODEC_VERSION_MAJOR>53
  int linesize;
  int bytesDecoded = av_samples_get_buffer_size(
      &linesize,
      codecCtx_->channels,
      frame->nb_samples,
      codecCtx_->sample_fmt, 0);
#endif

  ALbyte *frameData;
#if LIBAVCODEC_VERSION_MAJOR>53
  if(resampleContext_!=NULL) {
    frameData = (ALbyte *)av_malloc(bytesDecoded*sizeof(uint8_t));
    avresample_convert(
        resampleContext_,
        (uint8_t **)&frameData,
        linesize,
        frame->nb_samples,
        (uint8_t **)frame->data,
        frame->linesize[0],
        frame->nb_samples);
    audioFrame->convertedFrame = frameData;
  }
  else {
#endif
    frameData = (ALbyte*) frame->data[0];
    audioFrame->convertedFrame = NULL;
#if LIBAVCODEC_VERSION_MAJOR>53
  }
#endif

  // add a audio buffer to the OpenAL audio source
  audioFrame->buffer->set_data(alFormat_, frameData, bytesDecoded, rate_);
  audioSource_->queue(*audioFrame->buffer);
  frame->opaque = audioFrame;

  // (re)start playing. playback may have stop when all frames consumed.
  if(audioSource_->state() != AL_PLAYING) audioSource_->play();

  av_free_packet(packet);
  pushFrame(frame, bytesDecoded);
}

void AudioStream::AudioFrame::free()
{
  delete buffer;
  av_free(avFrame);
  if(convertedFrame) av_free(convertedFrame);
}

const ref_ptr<AudioSource>& AudioStream::audioSource()
{ return audioSource_; }
