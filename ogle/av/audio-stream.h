/*
 * audio-stream.h
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#ifndef AUDIO_STREAM_H_
#define AUDIO_STREAM_H_

extern "C" {
  #include <libavcodec/version.h>
#if LIBAVCODEC_VERSION_MAJOR>53
  #include <libavresample/avresample.h>
  #include <libavutil/opt.h>
#endif
}

#include <ogle/av/av-stream.h>
#include <ogle/av/audio-source.h>
#include <ogle/av/audio-buffer.h>

#include <ogle/utility/ref-ptr.h>

namespace ogle {
/**
 * \brief ffmpeg stream that provides OpenAL audio source.
 */
class AudioStream : public AudioVideoStream
{
public:
  /**
   * @param stream the stream object.
   * @param index the stream index.
   * @param chachedBytesLimit size limit for pre loading.
   */
  AudioStream(AVStream *stream, GLint index, GLuint chachedBytesLimit);
  virtual ~AudioStream();

  /**
   * OpenAL audio source.
   */
  const ref_ptr<AudioSource>& audioSource();

  // override
  void decode(AVPacket *packet);
  void clearQueue();

protected:
  struct AudioFrame {
#if LIBAVCODEC_VERSION_MAJOR>53
    AVFrame *avFrame;
#else
    uint8_t *avFrame;
#endif
    AudioBuffer *buffer;
    ALbyte *convertedFrame;
    void free();
  };

  ref_ptr<AudioSource> audioSource_;
#if LIBAVCODEC_VERSION_MAJOR>53
  struct AVAudioResampleContext *resampleContext_;
#endif
  ALenum alType_;
  ALenum alChannelLayout_;
  ALenum alFormat_;
  ALint rate_;

};
} // end ogle namespace

#endif /* AUDIO_STREAM_H_ */
