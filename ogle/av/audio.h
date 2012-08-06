/*
 * audio.h
 *
 *  Created on: 07.04.2012
 *      Author: daniel
 */

#ifndef AUDIO_H_
#define AUDIO_H_

#include <AL/al.h>    // OpenAL header files
#include <AL/alc.h>

#include <ogle/algebra/vector.h>

/**
 * General 3D audio configurations.
 */
class AudioSystem
{
public:
  static AudioSystem& get();

  /**
   * This function selects the OpenAL distance model. AL_INVERSE_DISTANCE,
   * AL_INVERSE_DISTANCE_CLAMPED, AL_LINEAR_DISTANCE, AL_LINEAR_DISTANCE_CLAMPED,
   * AL_EXPONENT_DISTANCE, AL_EXPONENT_DISTANCE_CLAMPED, or AL_NONE.
   */
  ALenum distanceModel() const;
  /**
   * OpenAL distance model. AL_INVERSE_DISTANCE,
   * AL_INVERSE_DISTANCE_CLAMPED, AL_LINEAR_DISTANCE, AL_LINEAR_DISTANCE_CLAMPED,
   * AL_EXPONENT_DISTANCE, AL_EXPONENT_DISTANCE_CLAMPED, or AL_NONE.
   */
  void set_distanceModel(ALenum v);
  /**
   * Doppler factor (default 1.0)
   */
  float dopplerFactor() const;
  /**
   * Doppler factor (default 1.0)
   */
  void set_dopplerFactor(float v);
  /**
   * speed of sound (default value 343.3)
   */
  float speedOfSound() const;
  /**
   * speed of sound (default value 343.3)
   */
  void set_speedOfSound(float v);

  /**
   * 'master gain' value should be positive
   */
  float gain() const;
  /**
   * 'master gain' value should be positive
   */
  void set_gain(const float &v);
  /**
   * X, Y, Z position
   */
  Vec3f listenerPosition() const;
  /**
   * X, Y, Z position
   */
  void set_listenerPosition(const Vec3f &pos);
  /**
   * velocity vector
   */
  Vec3f listenerVelocity() const;
  /**
   * velocity vector
   */
  void set_listenerVelocity(const Vec3f &vel);
  /**
   * orientation expressed as at and up vectors
   */
  Vec3f listenerOrientationAtVector() const;
  /**
   * orientation expressed as at and up vectors
   */
  Vec3f listenerOrientationUpVector() const;
  /**
   * orientation expressed as at and up vectors
   */
  void set_listenerOrientation(const Vec3f &at, const Vec3f &up);

private:
  static AudioSystem *instance_;

  AudioSystem();
  ~AudioSystem();
};

#endif /* AUDIO_H_ */
