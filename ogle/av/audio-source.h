/*
 * audio-source.h
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#ifndef AUDIO_SOURCE_H_
#define AUDIO_SOURCE_H_

#include <AL/al.h>    // OpenAL header files
#include <AL/alc.h>

#include <ogle/av/audio-buffer.h>
#include <ogle/algebra/vector.h>

/**
 * Source of audio in 3D space.
 */
class AudioSource
{
public:
  AudioSource();
  virtual ~AudioSource();

  const unsigned int id() const { return id_; }

  ALint state() const;

  void play();
  void stop();
  void rewind();
  void pause();

  void attach(AudioBuffer &buffer);
  /**
   * This function queues a set of buffers on a source.
   * All buffers attached to a source will be played in sequence.
   */
  void queue(AudioBuffer &buffer);
  void unqueue(AudioBuffer &buffer);

  /**
   * pitch multiplier. always positive
   */
  float pitch() const;
  void set_pitch(const float &v);
  /**
   * source gain. value should be positive
   */
  float gain() const;
  void set_gain(const float &v);
  /**
   * the minimum gain for this source
   */
  float minGain() const;
  void set_minGain(const float &v);
  /**
   * the maximum gain for this source
   */
  float maxGain() const;
  void set_maxGain(const float &v);
  /**
   * used with the Inverse Clamped Distance Model
   * to set the distance where there will no longer be
   * any attenuation of the source
   */
  float maxDistance() const;
  void set_maxDistance(const float &v);
  /**
   * the rolloff rate for the source. default is 1.0
   */
  float rolloffFactor() const;
  void set_rolloffFactor(const float &v);
  /**
   * the distance under which the volume for the
   * source would normally drop by half (before
   * being influenced by rolloff factor or maxDistance)
   */
  float referenceDistance() const;
  void set_referenceDistance(const float &v);
  /**
   * the gain when outside the oriented cone
   */
  float coneOuterGain() const;
  void set_coneOuterGain(const float &v);
  /**
   * the gain when inside the oriented cone
   */
  float coneInnerGain() const;
  void set_coneInnerGain(const float &v);
  /**
   * outer angle of the sound cone, in degrees. default is 360
   */
  float coneOuterAngle() const;
  void set_coneOuterAngle(const float &v);
  /**
   * determines if the positions are relative to the listener. default is false
   */
  bool sourceRelative() const;
  void set_sourceRelative(const bool &v);
  /**
   * the playback position, expressed in seconds.
   */
  float secOffset() const;
  void set_secOffset(const float &v);
  /**
   * the playback position, expressed in samples
   */
  int sampleOffset() const;
  void set_sampleOffset(const int &v);
  /**
   * the playback position, expressed in bytes
   */
  float byteOffset() const;
  void set_byteOffset(const float &v);
  /**
   * turns looping on (AL_TRUE) or off (AL_FALSE)
   */
  bool looping() const;
  void set_looping(const bool &v);
  /**
   * X, Y, Z position
   */
  Vec3f position() const;
  void set_position(const Vec3f &pos);
  /**
   * velocity vector
   */
  Vec3f velocity() const;
  void set_velocity(const Vec3f &vel);

protected:
  unsigned int id_;

private:
  AudioSource(const AudioSource&);
  AudioSource& operator=(const AudioSource&);
};

#endif /* AUDIO_SOURCE_H_ */
