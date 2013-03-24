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

#include <regen/av/av-stream.h>
#include <regen/av/audio-source.h>
#include <regen/av/audio-buffer.h>

#include <regen/utility/ref-ptr.h>

namespace regen {
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
   * @return elapsed time of last finished audio frame.
   */
  GLdouble elapsedTime() const;

  /**
   * OpenAL audio source.
   */
  const ref_ptr<AudioSource>& audioSource();

  // override
  void decode(AVPacket *packet);
  void clearQueue();

protected:
  struct AudioFrame {
    AVFrame *avFrame;
    AudioBuffer *buffer;
    ALbyte *convertedFrame;
    GLdouble dts;
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
  GLdouble elapsedTime_;

};
} // namespace

#endif /* AUDIO_STREAM_H_ */
