/*
 * audio-stream.h
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#ifndef AUDIO_STREAM_H_
#define AUDIO_STREAM_H_

#include <ogle/av/av-stream.h>
#include <ogle/av/audio-source.h>
#include <ogle/av/audio-buffer.h>

#include <ogle/utility/ref-ptr.h>

namespace ogle {
/**
 * \brief libav stream that provides OpenAL audio source.
 */
class AudioStream : public AudioVideoStream
{
public:
  AudioStream(AVStream *stream, int index, unsigned int chachedBytesLimit);
  virtual ~AudioStream();

  /**
   * OpenAL audio source.
   */
  const ref_ptr<AudioSource>& audioSource();

  // override
  void decode(AVPacket *packet);
  void clearQueue();

protected:
  static int64_t basetime_;
  static int64_t filetime_;

  ref_ptr<AudioSource> audioSource_;
  ALenum alType_;
  ALenum alChannelLayout_;
  ALenum alFormat_;
  ALint rate_;

};
} // end ogle namespace

#endif /* AUDIO_STREAM_H_ */
