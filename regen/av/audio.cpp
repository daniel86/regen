/*
 * audio.cpp
 *
 *  Created on: 07.04.2012
 *      Author: daniel
 */

#include <AL/alut.h>

#include "audio.h"
using namespace regen;

AudioSystem *AudioSystem::instance_ = NULL;

AudioSystem& AudioSystem::get()
{
  if(!instance_) {
    instance_ = new AudioSystem;
  }
  return *instance_;
}

AudioSystem::AudioSystem()
{
  alutInit(NULL, NULL);
  set_dopplerFactor( 1.0f );
  set_gain( 1.0f );
  set_speedOfSound( 343.3f );
  set_distanceModel( AL_LINEAR_DISTANCE );
}
AudioSystem::~AudioSystem()
{
  alutExit();
}

ALenum AudioSystem::distanceModel() const
{
  return alGetInteger(AL_DISTANCE_MODEL);
}
void AudioSystem::set_distanceModel(ALenum v)
{
  alDistanceModel(v);
}
ALfloat AudioSystem::dopplerFactor() const
{
  return alGetFloat(AL_DOPPLER_FACTOR);
}
void AudioSystem::set_dopplerFactor(ALfloat v)
{
  alDopplerFactor(v);
}
ALfloat AudioSystem::speedOfSound() const
{
  return alGetFloat(AL_SPEED_OF_SOUND);
}
void AudioSystem::set_speedOfSound(ALfloat v)
{
  alSpeedOfSound(v);
}

ALfloat AudioSystem::gain() const
{
  float v; alGetListenerf(AL_GAIN, &v); return v;
}
void AudioSystem::set_gain(ALfloat v)
{
  alListenerf(AL_GAIN, v);
}
Vec3f AudioSystem::listenerPosition() const
{
  Vec3f v; alGetListenerfv(AL_POSITION, &v.x); return v;
}
void AudioSystem::set_listenerPosition(const Vec3f &pos)
{
  alListenerfv(AL_POSITION, &pos.x);
}
Vec3f AudioSystem::listenerVelocity() const
{
  Vec3f v; alGetListenerfv(AL_VELOCITY, &v.x); return v;
}
void AudioSystem::set_listenerVelocity(const Vec3f &vel)
{
  alListenerfv(AL_VELOCITY, &vel.x);
}
Vec3f AudioSystem::listenerOrientationAtVector() const
{
  float v[6]; alGetListenerfv(AL_ORIENTATION, v); return *( (Vec3f*) v );
}
Vec3f AudioSystem::listenerOrientationUpVector() const
{
  float v[6]; alGetListenerfv(AL_ORIENTATION, v); return *( (Vec3f*) v+3 );
}
void AudioSystem::set_listenerOrientation(const Vec3f &to, const Vec3f &up)
{
  float v[6] = {to.x, to.y, to.z, up.x, up.y, up.z};
  alListenerfv(AL_ORIENTATION, v);
}
