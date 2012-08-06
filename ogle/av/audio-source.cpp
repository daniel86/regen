/*
 * audio-source.cpp
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#include "audio-source.h"

AudioSource::AudioSource()
{
  alGenSources(1, &id_);
}
AudioSource::~AudioSource()
{
  alDeleteSources(1, &id_);
}

ALint AudioSource::state() const
{
  ALint state;
  alGetSourcei(id_, AL_SOURCE_STATE, &state);
  return state;
}

void AudioSource::play()
{
  alSourcePlay(id_);
}
void AudioSource::stop()
{
  alSourceStop(id_);
}
void AudioSource::rewind()
{
  alSourceRewind(id_);
}
void AudioSource::pause()
{
  alSourcePause(id_);
}

void AudioSource::attach(AudioBuffer &buffer)
{
  alSourcei(id_, AL_BUFFER, buffer.id());
}
void AudioSource::queue(AudioBuffer &buffer)
{
  unsigned int buf = buffer.id();
  alSourceQueueBuffers(id_, 1, &buf);
}
void AudioSource::unqueue(AudioBuffer &buffer)
{
  unsigned int buf = buffer.id();
  alSourceUnqueueBuffers(id_, 1, &buf);
}

bool AudioSource::looping() const
{
  float v; alGetSourcef(id_, AL_LOOPING, &v); return v;
}
void AudioSource::set_looping(const bool &v)
{
  alSourcei(id_, AL_LOOPING, v);
}
float AudioSource::pitch() const
{
  float v; alGetSourcef(id_, AL_PITCH, &v); return v;
}
void AudioSource::set_pitch(const float &v)
{
  alSourcef(id_, AL_PITCH, v);
}
float AudioSource::minGain() const
{
  float v; alGetSourcef(id_, AL_MIN_GAIN, &v); return v;
}
void AudioSource::set_minGain(const float &v)
{
  alSourcef(id_, AL_MIN_GAIN, v);
}
float AudioSource::maxGain() const
{
  float v; alGetSourcef(id_, AL_MAX_GAIN, &v); return v;
}
void AudioSource::set_maxGain(const float &v)
{
  alSourcef(id_, AL_MAX_GAIN, v);
}
float AudioSource::gain() const
{
  float v; alGetSourcef(id_, AL_GAIN, &v); return v;
}
void AudioSource::set_gain(const float &v)
{
  alSourcef(id_, AL_GAIN, v);
}
float AudioSource::rolloffFactor() const
{
  float v; alGetSourcef(id_, AL_ROLLOFF_FACTOR, &v); return v;
}
void AudioSource::set_rolloffFactor(const float &v)
{
  alSourcef(id_, AL_ROLLOFF_FACTOR, v);
}
float AudioSource::referenceDistance() const
{
  float v; alGetSourcef(id_, AL_REFERENCE_DISTANCE, &v); return v;
}
void AudioSource::set_referenceDistance(const float &v)
{
  alSourcef(id_, AL_REFERENCE_DISTANCE, v);
}
float AudioSource::coneOuterGain() const
{
  float v; alGetSourcef(id_, AL_CONE_OUTER_GAIN, &v); return v;
}
void AudioSource::set_coneOuterGain(const float &v)
{
  alSourcef(id_, AL_CONE_OUTER_GAIN, v);
}
float AudioSource::coneInnerGain() const
{
  float v; alGetSourcef(id_, AL_CONE_INNER_ANGLE, &v); return v;
}
void AudioSource::set_coneInnerGain(const float &v)
{
  alSourcef(id_, AL_CONE_INNER_ANGLE, v);
}
float AudioSource::coneOuterAngle() const
{
  float v; alGetSourcef(id_, AL_CONE_OUTER_ANGLE, &v); return v;
}
void AudioSource::set_coneOuterAngle(const float &v)
{
  alSourcef(id_, AL_CONE_OUTER_ANGLE, v);
}
float AudioSource::maxDistance() const
{
  float v; alGetSourcef(id_, AL_MAX_DISTANCE, &v); return v;
}
void AudioSource::set_maxDistance(const float &v)
{
  alSourcef(id_, AL_MAX_DISTANCE, v);
}
bool AudioSource::sourceRelative() const
{
  float v; alGetSourcef(id_, AL_SOURCE_RELATIVE, &v); return v;
}
void AudioSource::set_sourceRelative(const bool &v)
{
  alSourcei(id_, AL_SOURCE_RELATIVE, v);
}
float AudioSource::secOffset() const
{
  float v; alGetSourcef(id_, AL_SEC_OFFSET, &v); return v;
}
void AudioSource::set_secOffset(const float &v)
{
  alSourcef(id_, AL_SEC_OFFSET, v);
}
int AudioSource::sampleOffset() const
{
  int v; alGetSourcei(id_, AL_SAMPLE_OFFSET, &v); return v;
}
void AudioSource::set_sampleOffset(const int &v)
{
  alSourcef(id_, AL_SAMPLE_OFFSET, v);
}
float AudioSource::byteOffset() const
{
  int v; alGetSourcei(id_, AL_BYTE_OFFSET, &v); return v;
}
void AudioSource::set_byteOffset(const float &v)
{
  alSourcef(id_, AL_BYTE_OFFSET, v);
}
Vec3f AudioSource::position() const
{
  Vec3f v; alGetSourcef(id_, AL_POSITION, &v.x); return v;
}
void AudioSource::set_position(const Vec3f &pos)
{
  alSourcefv(id_, AL_POSITION, &pos.x);
}
Vec3f AudioSource::velocity() const
{
  Vec3f v; alGetSourcef(id_, AL_VELOCITY, &v.x); return v;
}
void AudioSource::set_velocity(const Vec3f &vel)
{
  alSourcefv(id_, AL_VELOCITY, &vel.x);
}
