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

#include <regen/av/audio-buffer.h>
#include <regen/algebra/vector.h>

namespace regen {
/**
 * \brief Source of audio in 3D space.
 */
class AudioSource
{
public:
  AudioSource();
  virtual ~AudioSource();

  /**
   * The audio source ID.
   */
  ALuint id() const;

  /**
   * Current state of this audio source (playing,paused,..)
   */
  ALint state() const;

  /**
   * Start playing.
   */
  void play();
  /**
   * Stop playing.
   */
  void stop();
  /**
   * Pause playing.
   */
  void pause();
  /**
   * Rewind to start position.
   */
  void rewind();

  /**
   * Attaches a single buffer to this source.
   */
  void attach(AudioBuffer &buffer);
  /**
   * This function queues a set of buffers on a source.
   * All buffers attached to a source will be played in sequence.
   */
  void queue(AudioBuffer &buffer);
  /**
   * Unqueues buffer that was queued previously.
   */
  void unqueue(AudioBuffer &buffer);

  /**
   * pitch multiplier. always positive
   */
  ALfloat pitch() const;
  /**
   * pitch multiplier. always positive
   */
  void set_pitch(const ALfloat &v);

  /**
   * source gain. value should be positive
   */
  ALfloat gain() const;
  /**
   * source gain. value should be positive
   */
  void set_gain(const ALfloat &v);

  /**
   * the minimum gain for this source
   */
  ALfloat minGain() const;
  /**
   * the minimum gain for this source
   */
  void set_minGain(const ALfloat &v);

  /**
   * the maximum gain for this source
   */
  ALfloat maxGain() const;
  /**
   * the maximum gain for this source
   */
  void set_maxGain(const ALfloat &v);

  /**
   * used with the Inverse Clamped Distance Model
   * to set the distance where there will no longer be
   * any attenuation of the source
   */
  ALfloat maxDistance() const;
  /**
   * used with the Inverse Clamped Distance Model
   * to set the distance where there will no longer be
   * any attenuation of the source
   */
  void set_maxDistance(const ALfloat &v);

  /**
   * the rolloff rate for the source. default is 1.0
   */
  ALfloat rolloffFactor() const;
  /**
   * the rolloff rate for the source. default is 1.0
   */
  void set_rolloffFactor(const ALfloat &v);

  /**
   * the distance under which the volume for the
   * source would normally drop by half (before
   * being influenced by rolloff factor or maxDistance)
   */
  ALfloat referenceDistance() const;
  /**
   * the distance under which the volume for the
   * source would normally drop by half (before
   * being influenced by rolloff factor or maxDistance)
   */
  void set_referenceDistance(const ALfloat &v);

  /**
   * the gain when outside the oriented cone
   */
  ALfloat coneOuterGain() const;
  /**
   * the gain when outside the oriented cone
   */
  void set_coneOuterGain(const ALfloat &v);

  /**
   * the gain when inside the oriented cone
   */
  ALfloat coneInnerGain() const;
  /**
   * the gain when inside the oriented cone
   */
  void set_coneInnerGain(const ALfloat &v);

  /**
   * outer angle of the sound cone, in degrees. default is 360
   */
  ALfloat coneOuterAngle() const;
  /**
   * outer angle of the sound cone, in degrees. default is 360
   */
  void set_coneOuterAngle(const ALfloat &v);

  /**
   * determines if the positions are relative to the listener. default is false
   */
  ALboolean sourceRelative() const;
  /**
   * determines if the positions are relative to the listener. default is false
   */
  void set_sourceRelative(const ALboolean &v);

  /**
   * the playback position, expressed in seconds.
   */
  ALfloat secOffset() const;
  /**
   * the playback position, expressed in seconds.
   */
  void set_secOffset(const ALfloat &v);

  /**
   * the playback position, expressed in samples
   */
  ALint sampleOffset() const;
  /**
   * the playback position, expressed in samples
   */
  void set_sampleOffset(const ALint &v);

  /**
   * the playback position, expressed in bytes
   */
  ALfloat byteOffset() const;
  /**
   * the playback position, expressed in bytes
   */
  void set_byteOffset(const ALfloat &v);

  /**
   * turns looping on (AL_TRUE) or off (AL_FALSE)
   */
  ALboolean looping() const;
  /**
   * turns looping on (AL_TRUE) or off (AL_FALSE)
   */
  void set_looping(const ALboolean &v);

  /**
   * X, Y, Z position
   */
  Vec3f position() const;
  /**
   * X, Y, Z position
   */
  void set_position(const Vec3f &pos);

  /**
   * velocity vector
   */
  Vec3f velocity() const;
  /**
   * velocity vector
   */
  void set_velocity(const Vec3f &vel);

protected:
  ALuint id_;

private:
  AudioSource(const AudioSource&);
  AudioSource& operator=(const AudioSource&);
};
} // namespace

#endif /* AUDIO_SOURCE_H_ */
