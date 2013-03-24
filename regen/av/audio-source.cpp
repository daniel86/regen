/*
 * audio-source.cpp
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#include "audio-source.h"
using namespace regen;

AudioSource::AudioSource()
{
  alGenSources(1, &id_);
}
AudioSource::~AudioSource()
{
  alDeleteSources(1, &id_);
}

ALuint AudioSource::id() const
{
  return id_;
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
  ALuint buf = buffer.id();
  alSourceQueueBuffers(id_, 1, &buf);
}
void AudioSource::unqueue(AudioBuffer &buffer)
{
  ALuint buf = buffer.id();
  alSourceUnqueueBuffers(id_, 1, &buf);
}

ALboolean AudioSource::looping() const
{
  ALint v; alGetSourcei(id_, AL_LOOPING, &v); return v;
}
void AudioSource::set_looping(const ALboolean &v)
{
  alSourcei(id_, AL_LOOPING, v);
}
ALfloat AudioSource::pitch() const
{
  ALfloat v; alGetSourcef(id_, AL_PITCH, &v); return v;
}
void AudioSource::set_pitch(const ALfloat &v)
{
  alSourcef(id_, AL_PITCH, v);
}
ALfloat AudioSource::minGain() const
{
  ALfloat v; alGetSourcef(id_, AL_MIN_GAIN, &v); return v;
}
void AudioSource::set_minGain(const ALfloat &v)
{
  alSourcef(id_, AL_MIN_GAIN, v);
}
ALfloat AudioSource::maxGain() const
{
  ALfloat v; alGetSourcef(id_, AL_MAX_GAIN, &v); return v;
}
void AudioSource::set_maxGain(const ALfloat &v)
{
  alSourcef(id_, AL_MAX_GAIN, v);
}
ALfloat AudioSource::gain() const
{
  ALfloat v; alGetSourcef(id_, AL_GAIN, &v); return v;
}
void AudioSource::set_gain(const ALfloat &v)
{
  alSourcef(id_, AL_GAIN, v);
}
ALfloat AudioSource::rolloffFactor() const
{
  ALfloat v; alGetSourcef(id_, AL_ROLLOFF_FACTOR, &v); return v;
}
void AudioSource::set_rolloffFactor(const ALfloat &v)
{
  alSourcef(id_, AL_ROLLOFF_FACTOR, v);
}
ALfloat AudioSource::referenceDistance() const
{
  ALfloat v; alGetSourcef(id_, AL_REFERENCE_DISTANCE, &v); return v;
}
void AudioSource::set_referenceDistance(const ALfloat &v)
{
  alSourcef(id_, AL_REFERENCE_DISTANCE, v);
}
ALfloat AudioSource::coneOuterGain() const
{
  ALfloat v; alGetSourcef(id_, AL_CONE_OUTER_GAIN, &v); return v;
}
void AudioSource::set_coneOuterGain(const ALfloat &v)
{
  alSourcef(id_, AL_CONE_OUTER_GAIN, v);
}
ALfloat AudioSource::coneInnerGain() const
{
  ALfloat v; alGetSourcef(id_, AL_CONE_INNER_ANGLE, &v); return v;
}
void AudioSource::set_coneInnerGain(const ALfloat &v)
{
  alSourcef(id_, AL_CONE_INNER_ANGLE, v);
}
ALfloat AudioSource::coneOuterAngle() const
{
  ALfloat v; alGetSourcef(id_, AL_CONE_OUTER_ANGLE, &v); return v;
}
void AudioSource::set_coneOuterAngle(const ALfloat &v)
{
  alSourcef(id_, AL_CONE_OUTER_ANGLE, v);
}
ALfloat AudioSource::maxDistance() const
{
  ALfloat v; alGetSourcef(id_, AL_MAX_DISTANCE, &v); return v;
}
void AudioSource::set_maxDistance(const ALfloat &v)
{
  alSourcef(id_, AL_MAX_DISTANCE, v);
}
ALboolean AudioSource::sourceRelative() const
{
  ALint v; alGetSourcei(id_, AL_SOURCE_RELATIVE, &v); return v;
}
void AudioSource::set_sourceRelative(const ALboolean &v)
{
  alSourcei(id_, AL_SOURCE_RELATIVE, v);
}
ALfloat AudioSource::secOffset() const
{
  ALfloat v; alGetSourcef(id_, AL_SEC_OFFSET, &v); return v;
}
void AudioSource::set_secOffset(const ALfloat &v)
{
  alSourcef(id_, AL_SEC_OFFSET, v);
}
ALint AudioSource::sampleOffset() const
{
  ALint v; alGetSourcei(id_, AL_SAMPLE_OFFSET, &v); return v;
}
void AudioSource::set_sampleOffset(const ALint &v)
{
  alSourcef(id_, AL_SAMPLE_OFFSET, v);
}
ALfloat AudioSource::byteOffset() const
{
  ALfloat v; alGetSourcef(id_, AL_BYTE_OFFSET, &v); return v;
}
void AudioSource::set_byteOffset(const ALfloat &v)
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
