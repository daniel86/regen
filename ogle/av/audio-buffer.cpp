/*
 * audio-buffer.cpp
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#include <AL/alut.h>

#include "audio-buffer.h"

AudioBuffer::AudioBuffer() : id_(0)
{
}
AudioBuffer::~AudioBuffer()
{
  if(id_!=0) alDeleteBuffers(1, &id_);
}

int AudioBuffer::frequency() const {
  int v; alGetBufferi(id_, AL_FREQUENCY, &v); return v;
}
void AudioBuffer::set_frequency(int v) {
  alBufferi(id_, AL_FREQUENCY, v);
}
int AudioBuffer::bits() const {
  int v; alGetBufferi(id_, AL_BITS, &v); return v;
}
void AudioBuffer::set_bits(int v) {
  alBufferi(id_, AL_BITS, v);
}
int AudioBuffer::channels() const {
  int v; alGetBufferi(id_, AL_CHANNELS, &v); return v;
}
void AudioBuffer::set_channels(int v) {
  alBufferi(id_, AL_CHANNELS, v);
}
int AudioBuffer::size() const {
  int v; alGetBufferi(id_, AL_SIZE, &v); return v;
}
void AudioBuffer::set_size(int v) {
  alBufferi(id_, AL_SIZE, v);
}

void AudioBuffer::set_data(ALenum format, ALbyte *data, int bytes, int rate)
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
void AudioBuffer::loadData(void *data, unsigned int length)
{
  id_ = alutCreateBufferFromFileImage(data, length);
}
void AudioBuffer::loadWaveform(
    ALenum waveshape,
    ALfloat frequency,
    ALfloat phase,
    ALfloat duration)
{
  id_ = alutCreateBufferWaveform(waveshape, frequency, phase, duration);
}
