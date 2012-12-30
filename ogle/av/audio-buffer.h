/*
 * audio-buffer.h
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#ifndef AUDIO_BUFFER_H_
#define AUDIO_BUFFER_H_

#include <AL/al.h>    // OpenAL header files
#include <AL/alc.h>

#include <string>
using namespace std;

/**
 * Buffer for audio data.
 * Can be associated to AudioSource's
 */
class AudioBuffer
{
public:
  AudioBuffer();
  virtual ~AudioBuffer();

  ALuint id() const { return id_; }

  /**
   * frequency of buffer in Hz
   */
  ALint frequency() const;
  /**
   * frequency of buffer in Hz
   */
  void set_frequency(ALint v);
  /**
   * bit depth of buffer
   */
  ALint bits() const;
  /**
   * bit depth of buffer
   */
  void set_bits(ALint v);
  /**
   * number of channels in buffer > 1 is valid,
   * but buffer won't be positioned when played
   */
  ALint channels() const;
  /**
   * number of channels in buffer > 1 is valid,
   * but buffer won't be positioned when played
   */
  void set_channels(ALint v);
  /**
   * size of buffer in bytes
   */
  ALint size() const;
  /**
   * size of buffer in bytes
   */
  void set_size(ALint v);

  void set_data(ALenum format, ALbyte *data, ALint bytes, ALint rate);

  void loadHelloWorld();
  void loadFile(const string &file);
  void loadData(ALvoid *data, ALuint length);
  void loadWaveform(ALenum waveshape, ALfloat frequency, ALfloat phase, ALfloat duration);

protected:
  ALuint id_;

private:
  AudioBuffer(const AudioBuffer&);
  AudioBuffer& operator=(const AudioBuffer&);
};

#endif /* AUDIO_BUFFER_H_ */
