/*
 * audio-source.cpp
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#include <AL/alut.h>
#include <regen/utility/string-util.h>
#include <regen/utility/logging.h>
#include "audio.h"
using namespace regen;

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

#ifndef AVCODEC_MAX_AUDIO_FRAME_SIZE
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
#endif

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
#ifdef HAS_LIBAVRESAMPLE
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
    throw new AudioSource::Error(REGEN_STRING(
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
    throw new AudioSource::Error(REGEN_STRING(
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
    default: throw new AudioSource::Error("unsupported format");
    }
    break;
  case AL_SHORT_SOFT:
    switch(layout) {
    case AL_MONO_SOFT:    return AL_FORMAT_MONO16;
    case AL_STEREO_SOFT:  return AL_FORMAT_STEREO16;
    case AL_QUAD_SOFT:    return alGetEnumValue("AL_FORMAT_QUAD16");
    case AL_5POINT1_SOFT: return alGetEnumValue("AL_FORMAT_51CHN16");
    case AL_7POINT1_SOFT: return alGetEnumValue("AL_FORMAT_71CHN16");
    default: throw new AudioSource::Error("unsupported format");
    }
    break;
  case AL_FLOAT_SOFT:
    switch(layout) {
    case AL_MONO_SOFT:    return alGetEnumValue("AL_FORMAT_MONO_FLOAT32");
    case AL_STEREO_SOFT:  return alGetEnumValue("AL_FORMAT_STEREO_FLOAT32");
    case AL_QUAD_SOFT:    return alGetEnumValue("AL_FORMAT_QUAD32");
    case AL_5POINT1_SOFT: return alGetEnumValue("AL_FORMAT_51CHN32");
    case AL_7POINT1_SOFT: return alGetEnumValue("AL_FORMAT_71CHN32");
    default: throw new AudioSource::Error("unsupported format");
    }
    break;
  case AL_DOUBLE_SOFT:
    switch(layout) {
    case AL_MONO_SOFT:    return alGetEnumValue("AL_FORMAT_MONO_DOUBLE");
    case AL_STEREO_SOFT:  return alGetEnumValue("AL_FORMAT_STEREO_DOUBLE");
    default: throw new AudioSource::Error("unsupported format");
    }
    break;
  default: throw new AudioSource::Error("unsupported format");
  }
}

///////////
///////////

ALboolean AudioLibrary::isALInitialized_ = AL_FALSE;

void AudioLibrary::initializeAL()
{
  if(isALInitialized_) return;
  alutInit(NULL, NULL);
  isALInitialized_ = AL_TRUE;
  set_dopplerFactor(1.0f);
  set_speedOfSound(343.3f);
  set_distanceModel(AL_LINEAR_DISTANCE);
  AudioListener::set1f(AL_GAIN,1.0f);
}
void AudioLibrary::closeAL()
{
  if(isALInitialized_) alutExit();
}

void AudioLibrary::set_distanceModel(ALenum v)
{ alDistanceModel(v); }
void AudioLibrary::set_dopplerFactor(ALfloat v)
{ alDopplerFactor(v); }
void AudioLibrary::set_speedOfSound(ALfloat v)
{ alSpeedOfSound(v); }

///////////
///////////

void AudioListener::set1f(const ALenum &p, const ALfloat &v)
{ alListenerfv(p, &v); }
ALfloat AudioListener::get1f(const ALenum &p)
{
  ALfloat v;
  alGetListenerf(p, &v);
  return v;
}

void AudioListener::set3f(const ALenum &p, const Vec3f &v)
{ alListenerfv(p, &v.x); }
Vec3f AudioListener::get3f(const ALenum &p)
{
  Vec3f v;
  alGetListenerf(p, &v.x);
  return v;
}

void AudioListener::set6f(const ALenum &p, const Vec6f &v)
{ alListenerfv(p, &v.x0); }
Vec6f AudioListener::get6f(const ALenum &p)
{
  Vec6f v;
  alGetListenerf(p, &v.x0);
  return v;
}

///////////
///////////

AudioSource::AudioBuffer::AudioBuffer()
{
  alGenBuffers(1, &id_);
}
AudioSource::AudioBuffer::~AudioBuffer()
{
  alDeleteBuffers(1, &id_);
}

AudioSource::AudioSource(GLuint chachedBytesLimit)
: AudioVideoStream(chachedBytesLimit),
  alType_(AL_NONE),
  alChannelLayout_(AL_NONE),
  alFormat_(AL_NONE),
  rate_(0)
{
  AudioLibrary::initializeAL();
  alGenSources(1, &id_);
}
AudioSource::AudioSource(AVStream *stream, GLint index, GLuint chachedBytesLimit)
: AudioVideoStream(chachedBytesLimit),
  alType_(AL_NONE),
  alChannelLayout_(AL_NONE),
  alFormat_(AL_NONE),
  rate_(0)
{
  AudioLibrary::initializeAL();
  alGenSources(1, &id_);
  openAudioStream(stream,index,GL_TRUE);
}
AudioSource::~AudioSource()
{
#ifdef HAS_LIBAVRESAMPLE
  if(resampleContext_) avresample_free(&resampleContext_);
#endif
  clearQueue();
  alDeleteSources(1, &id_);
}

ALuint AudioSource::id() const
{ return id_; }
GLdouble AudioSource::elapsedTime() const
{ return elapsedTime_; }

void AudioSource::set1i(const ALenum &p, const ALint &v)
{ alSourcei(id_, p, v); }
ALint AudioSource::get1i(const ALenum &p)
{
  ALint v;
  alGetSourcei(id_, p, &v);
  return v;
}

void AudioSource::set1f(const ALenum &p, const ALfloat &v)
{ alSourcef(id_, p, v); }
ALfloat AudioSource::get1f(const ALenum &p)
{
  ALfloat v;
  alGetSourcef(id_, p, &v);
  return v;
}

void AudioSource::set3f(const ALenum &p, const Vec3f &v)
{ alSourcefv(id_, p, &v.x); }
Vec3f AudioSource::get3f(const ALenum &p)
{
  Vec3f v;
  alGetSourcef(id_, p, &v.x);
  return v;
}

void AudioSource::play()
{ alSourcePlay(id_); }
void AudioSource::stop()
{ alSourceStop(id_); }
void AudioSource::rewind()
{ alSourceRewind(id_); }
void AudioSource::pause()
{ alSourcePause(id_); }

void AudioSource::push(const ref_ptr<AudioBuffer> &buffer)
{
  queued_.push(buffer);
  alSourceQueueBuffers(id_, 1, &buffer->id_);
}
void AudioSource::pop()
{
  const ref_ptr<AudioBuffer> &buffer = queued_.top();
  alSourceUnqueueBuffers(id_, 1, &buffer->id_);
  queued_.pop();
}

//////////////
//////////////

void AudioSource::openAudioStream(AVStream *stream, GLint index, GLboolean initial)
{
  if(!initial) {
#ifdef HAS_LIBAVRESAMPLE
    if(resampleContext_) avresample_free(&resampleContext_);
#endif
    clearQueue();
  }

  AudioVideoStream::open(stream,index,initial);
  alType_ = avToAlType(codecCtx_->sample_fmt);
  alChannelLayout_ = avToAlLayout(codecCtx_->channel_layout);
  alFormat_ = avFormat(alType_, alChannelLayout_);
  rate_ = codecCtx_->sample_rate;

  REGEN_DEBUG("init audio stream" <<
      " AL format=" << alFormat_ <<
      " sample_fmt=" << codecCtx_->sample_fmt <<
      " channel_layout=" << codecCtx_->channel_layout <<
      " sample_rate=" << codecCtx_->sample_rate <<
      " bit_rate=" << codecCtx_->bit_rate <<
      ".");
#ifdef HAS_LIBAVRESAMPLE
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
    REGEN_DEBUG("converting sample format to " << out_sample_fmt << ".");
  }
#endif
}

void AudioSource::clearQueue()
{
  stop();
  alSourcei(id_, AL_BUFFER, 0);

  while(decodedFrames_.size()>0) {
    AVFrame *f = frontFrame(); popFrame();
    AudioFrame *buf = (AudioFrame*)f->opaque;
    buf->free();
    delete buf;
    pop();
  }
}

void AudioSource::decode(AVPacket *packet)
{
  AVFrame *frame = avcodec_alloc_frame();
  // Decode audio frame
#ifdef HAS_LIBAVRESAMPLE
  int frameFinished = 0;
  avcodec_decode_audio4(codecCtx_, frame, &frameFinished, packet);
  if(!frameFinished) {
    av_free(frame);
    return;
  }
#else
  int bytesDecoded = AVCODEC_MAX_AUDIO_FRAME_SIZE;
  int16_t *frameData = (int16_t*)av_malloc(AVCODEC_MAX_AUDIO_FRAME_SIZE);
  avcodec_decode_audio3(codecCtx_, frameData, &bytesDecoded, packet);
#endif

  // unqueue processed buffers
  ALint processed;
  alGetSourcei(id_, AL_BUFFERS_PROCESSED, &processed);
  for(; processed>0; --processed)
  {
    ALuint bufid;
    alSourceUnqueueBuffers(id_, 1, &bufid);
    AVFrame *processedFrame = frontFrame(); popFrame();
    AudioFrame *af = (AudioFrame*)processedFrame->opaque;
    elapsedTime_ = af->dts;
    af->free();
    delete af;
  }

  AudioFrame *audioFrame = new AudioFrame;
  audioFrame->avFrame = frame;
  audioFrame->buffer = ref_ptr<AudioBuffer>::alloc();
  audioFrame->dts = (packet->pts + packet->duration)*av_q2d(stream_->time_base);
  // Get the required buffer size for the given audio parameters.
#ifdef HAS_LIBAVRESAMPLE
  int linesize;
  int bytesDecoded = av_samples_get_buffer_size(
      &linesize,
      codecCtx_->channels,
      frame->nb_samples,
      codecCtx_->sample_fmt, 0);
#endif

#ifdef HAS_LIBAVRESAMPLE
  ALbyte *frameData;
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
    frameData = (ALbyte*) frame->data[0];
    audioFrame->convertedFrame = NULL;
  }
#else
  audioFrame->convertedFrame = (ALbyte*) frameData;
#endif

  // add a audio buffer to the OpenAL audio source
  alBufferData(audioFrame->buffer->id_,
      alFormat_, (ALbyte*)frameData, bytesDecoded, rate_);
  push(audioFrame->buffer);
  frame->opaque = audioFrame;

  // (re)start playing. playback may have stop when all frames consumed.
  if(get1i(AL_SOURCE_STATE) != AL_PLAYING) play();

  av_free_packet(packet);
  pushFrame(frame, bytesDecoded);
}

void AudioSource::AudioFrame::free()
{
  av_free(avFrame);
  if(convertedFrame) av_free(convertedFrame);
}
