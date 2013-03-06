/*
 * audio-buffer.cpp
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#include <AL/alut.h>

#include "audio-buffer.h"
using namespace ogle;

AudioBuffer::AudioBuffer() : id_(0)
{
}
AudioBuffer::~AudioBuffer()
{
  if(id_!=0) alDeleteBuffers(1, &id_);
}

ALuint AudioBuffer::id() const
{
  return id_;
}

int AudioBuffer::frequency() const
{
  ALint v; alGetBufferi(id_, AL_FREQUENCY, &v); return v;
}
void AudioBuffer::set_frequency(ALint v)
{
  alBufferi(id_, AL_FREQUENCY, v);
}
ALint AudioBuffer::bits() const
{
  ALint v; alGetBufferi(id_, AL_BITS, &v); return v;
}
void AudioBuffer::set_bits(ALint v)
{
  alBufferi(id_, AL_BITS, v);
}
ALint AudioBuffer::channels() const
{
  ALint v; alGetBufferi(id_, AL_CHANNELS, &v); return v;
}
void AudioBuffer::set_channels(ALint v)
{
  alBufferi(id_, AL_CHANNELS, v);
}
ALint AudioBuffer::size() const
{
  ALint v; alGetBufferi(id_, AL_SIZE, &v); return v;
}
void AudioBuffer::set_size(ALint v)
{
  alBufferi(id_, AL_SIZE, v);
}

void AudioBuffer::set_data(ALenum format, ALbyte *data, ALint bytes, ALint rate)
{
  if(id_==0) {
    alGenBuffers(1, &id_);
  }
  alBufferData(id_, format, data, bytes, rate);
}

void AudioBuffer::loadHelloWorld()
{
  id_ = alutCreateBufferHelloWorld();
}
void AudioBuffer::loadFile(const string &file)
{
  id_ = alutCreateBufferFromFile(file.c_str());
}
void AudioBuffer::loadData(ALvoid *data, ALuint length)
{
  id_ = alutCreateBufferFromFileImage(data, length);
}
void AudioBuffer::loadWaveform(ALenum waveshape, ALfloat frequency, ALfloat phase, ALfloat duration)
{
  id_ = alutCreateBufferWaveform(waveshape, frequency, phase, duration);
}
