/*
 * audio-stream.cpp
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */


#include <AL/al.h>    // OpenAL header files
#include <AL/alc.h>
#include <AL/alext.h>

#include "audio-stream.h"

#ifndef AL_BYTE
// Sample types
#define AL_BYTE                                  0x1400
#define AL_UNSIGNED_BYTE                         0x1401
#define AL_SHORT                                 0x1402
#define AL_UNSIGNED_SHORT                        0x1403
#define AL_INT                                   0x1404
#define AL_UNSIGNED_INT                          0x1405
#define AL_FLOAT                                 0x1406
#define AL_DOUBLE                                0x1407
// Channel configurations
#define AL_MONO                                  0x1500
#define AL_STEREO                                0x1501
#define AL_REAR                                  0x1502
#define AL_QUAD                                  0x1503
#define AL_5POINT1                               0x1504 /* (WFX order) */
#define AL_6POINT1                               0x1505 /* (WFX order) */
#define AL_7POINT1                               0x1506 /* (WFX order) */
#endif

static ALenum avToAlType(AVSampleFormat format)
{
  switch(format)
  {
  case AV_SAMPLE_FMT_U8:
    return AL_UNSIGNED_BYTE;
  case AV_SAMPLE_FMT_S16:
    return AL_SHORT;
  case AV_SAMPLE_FMT_S32:
    return AL_INT;
  case AV_SAMPLE_FMT_FLT:
    return AL_FLOAT;
  case AV_SAMPLE_FMT_DBL:
    return AL_DOUBLE;
  default:
    return AL_UNSIGNED_BYTE;
  }
}
static ALenum avToAlLayout(uint64_t layout)
{
  switch(layout)
  {
  case AV_CH_LAYOUT_MONO:
    return AL_MONO;
  case AV_CH_LAYOUT_STEREO:
    return AL_STEREO;
  case AV_CH_LAYOUT_QUAD:
    return AL_QUAD;
  case AV_CH_LAYOUT_5POINT1:
    return AL_5POINT1;
  case AV_CH_LAYOUT_7POINT1:
    return AL_7POINT1;
  default:
    return AL_STEREO;
  }
}
static ALenum avFormat(ALenum type, ALenum layout)
{
  switch(type)
  {
  case AL_UNSIGNED_BYTE:
    switch(layout) {
    case AL_MONO:    return AL_FORMAT_MONO8;
    case AL_STEREO:  return AL_FORMAT_STEREO8;
    case AL_QUAD:    return alGetEnumValue("AL_FORMAT_QUAD8");
    case AL_5POINT1: return alGetEnumValue("AL_FORMAT_51CHN8");
    case AL_7POINT1: return alGetEnumValue("AL_FORMAT_71CHN8");
    default: throw new AudioStreamError("unsupported format");
    }
  case AL_SHORT:
    switch(layout) {
    case AL_MONO:    return AL_FORMAT_MONO16;
    case AL_STEREO:  return AL_FORMAT_STEREO16;
    case AL_QUAD:    return alGetEnumValue("AL_FORMAT_QUAD16");
    case AL_5POINT1: return alGetEnumValue("AL_FORMAT_51CHN16");
    case AL_7POINT1: return alGetEnumValue("AL_FORMAT_71CHN16");
    default: throw new AudioStreamError("unsupported format");
    }
  case AL_FLOAT:
    switch(layout) {
    case AL_MONO:    return alGetEnumValue("AL_FORMAT_MONO_FLOAT32");
    case AL_STEREO:  return alGetEnumValue("AL_FORMAT_STEREO_FLOAT32");
    case AL_QUAD:    return alGetEnumValue("AL_FORMAT_QUAD32");
    case AL_5POINT1: return alGetEnumValue("AL_FORMAT_51CHN32");
    case AL_7POINT1: return alGetEnumValue("AL_FORMAT_71CHN32");
    default: throw new AudioStreamError("unsupported format");
    }
  case AL_DOUBLE:
    switch(layout) {
    case AL_MONO:    return alGetEnumValue("AL_FORMAT_MONO_DOUBLE");
    case AL_STEREO:  return alGetEnumValue("AL_FORMAT_STEREO_DOUBLE");
    default: throw new AudioStreamError("unsupported format");
    }
  default: throw new AudioStreamError("unsupported format");
  }
}

AudioStream::AudioStream(AVStream *stream,
    int index,
    unsigned int chachedBytesLimit)
: AudioVideoStream(stream, index, chachedBytesLimit),
  audioSource_( ref_ptr<AudioSource>::manage( new AudioSource ) ),
  alType_( avToAlType(codecCtx_->sample_fmt) ),
  alChannelLayout_( avToAlLayout(codecCtx_->channel_layout) ),
  alFormat_( avFormat(alType_, alChannelLayout_) ),
  rate_( codecCtx_->sample_rate )
{
}
AudioStream::~AudioStream()
{
  clearQueue();
}

const ref_ptr<AudioSource>& AudioStream::audioSource()
{
  return audioSource_;
}

void AudioStream::clearQueue()
{
  alSourceStop(audioSource_->id());
  alSourcei(audioSource_->id(), AL_BUFFER, 0);

  while(decodedFrames_.size()>0) {
    AVFrame *f = frontFrame();
    AudioBuffer *buf = (AudioBuffer*)f->opaque;
    audioSource_->unqueue(*buf);
    delete buf;
    popFrame();
    av_free(f);
  }
}

void AudioStream::decode(AVPacket *packet)
{
  AVFrame *frame = avcodec_alloc_frame();
  int frameFinished = 0;
  // Decode video frame
  avcodec_decode_audio4(codecCtx_, frame, &frameFinished, packet);
  // Did we get a audio frame?
  if(!frameFinished) {
    av_free(frame);
    return;
  }
  // Get the required buffer size for the given audio parameters.
  int bytesDecoded = av_samples_get_buffer_size(NULL, codecCtx_->channels,
      frame->nb_samples, codecCtx_->sample_fmt, 1);
  // unqueue processed buffers
  ALint processed;
  alGetSourcei(audioSource_->id(), AL_BUFFERS_PROCESSED, &processed);
  while(processed > 0)
  {
    ALuint bufid;
    alSourceUnqueueBuffers(audioSource_->id(), 1, &bufid);
    processed -= 1;
    AVFrame *frame = frontFrame();
    popFrame();
    delete (AudioBuffer*)frame->opaque;
    av_free(frame);
  }

  // add a audio buffer to the OpenAL audio source
  AudioBuffer *alBuffer = new AudioBuffer;
  alBuffer->set_data( alFormat_, (ALbyte*) frame->data[0], bytesDecoded, rate_ );
  audioSource_->queue(*alBuffer);
  frame->opaque = alBuffer;

  // (re)start playing. playback may have stop when all frames consumed.
  if(audioSource_->state() != AL_PLAYING) audioSource_->play();

  av_free_packet(packet);
  pushFrame(frame, bytesDecoded);
}
