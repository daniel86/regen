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

namespace ogle {

/**
 * \brief General 3D audio configurations.
 */
class AudioSystem
{
public:
  /**
   * @return the AudioSystem singleton.
   */
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
  ALfloat dopplerFactor() const;
  /**
   * Doppler factor (default 1.0)
   */
  void set_dopplerFactor(ALfloat v);
  /**
   * speed of sound (default value 343.3)
   */
  ALfloat speedOfSound() const;
  /**
   * speed of sound (default value 343.3)
   */
  void set_speedOfSound(ALfloat v);

  /**
   * 'master gain' value should be positive
   */
  ALfloat gain() const;
  /**
   * 'master gain' value should be positive
   */
  void set_gain(ALfloat v);
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

} // end ogle namespace

#endif /* AUDIO_H_ */
