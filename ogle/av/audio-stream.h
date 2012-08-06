/*
 * audio-stream.h
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#ifndef AUDIO_STREAM_H_
#define AUDIO_STREAM_H_

#include <ogle/av/ffmpeg-stream.h>
#include <ogle/av/audio-source.h>
#include <ogle/av/audio-buffer.h>

#include <ogle/utility/ref-ptr.h>

/* Define the number of buffers and buffer size (in bytes) to use. 3 buffers is
 * a good amount (one playing, one ready to play, another being filled). 32256
 * is a good length per buffer, as it fits 1, 2, 4, 6, 7, 8, 12, 14, 16, 24,
 * 28, and 32 bytes-per-frame sizes. */
#define NUM_AUDIO_STREAM_BUFFERS 3
#define AUDIO_STREAM_BUFFER_SIZE 32256

class AudioStreamError : public runtime_error {
public: AudioStreamError(const string &message) : runtime_error(message) {}
};

class AudioStream : public LibAVStream
{
public:
  AudioStream(AVStream *stream,
      int index,
      unsigned int chachedBytesLimit);
  ~AudioStream();

  /**
   * OpenAL audio source.
   */
  ref_ptr<AudioSource> audioSource() { return audioSource_; }

  // FFMpegStream override
  virtual void decode(AVPacket *packet);

protected:
  int rate_;
  static int64_t basetime_;
  static int64_t filetime_;

  ref_ptr<AudioSource> audioSource_;
  ALenum alFormat_;
  ALenum alType_;
  ALenum alChannelLayout_;

};

#endif /* AUDIO_STREAM_H_ */
