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

  const unsigned int id() const { return id_; }

  /**
   * frequency of buffer in Hz
   */
  int frequency() const;
  /**
   * frequency of buffer in Hz
   */
  void set_frequency(int v);
  /**
   * bit depth of buffer
   */
  int bits() const;
  /**
   * bit depth of buffer
   */
  void set_bits(int v);
  /**
   * number of channels in buffer > 1 is valid,
   * but buffer won't be positioned when played
   */
  int channels() const;
  /**
   * number of channels in buffer > 1 is valid,
   * but buffer won't be positioned when played
   */
  void set_channels(int v);
  /**
   * size of buffer in bytes
   */
  int size() const;
  /**
   * size of buffer in bytes
   */
  void set_size(int v);

  void set_data(
      ALenum format,
      ALbyte *data,
      int bytes,
      int rate);

  void loadHelloWorld();
  void loadFile(const string &file);
  void loadData(void *data, unsigned int length);
  void loadWaveform(
      ALenum waveshape,
      ALfloat frequency,
      ALfloat phase,
      ALfloat duration);

protected:
  unsigned int id_;

private:
  AudioBuffer(const AudioBuffer&);
  AudioBuffer& operator=(const AudioBuffer&);
};

#endif /* AUDIO_BUFFER_H_ */
