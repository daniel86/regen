/*
 * vbo-wave-animation.h
 *
 *  Created on: 23.11.2011
 *      Author: daniel
 */

#ifndef VBO_WAVE_ANIMATION_H_
#define VBO_WAVE_ANIMATION_H_

#include <ogle/animations/vbo-animation.h>

class AnimationWave;

/**
 * Change vertex data based on sinus equations.
 */
class VBOWaveAnimation : public VBOAnimation
{
public:
  VBOWaveAnimation(
      ref_ptr<MeshState> &p,
      GLboolean animateNormal=true);

  /**
   * Adds a wave to this animation
   */
  void addWave(ref_ptr<AnimationWave> wave);
  /**
   * Removes a previously added wave.
   */
  void removeWave(AnimationWave *wave);

  // override
  virtual GLboolean animateVBO(GLdouble dt);

protected:
  list< ref_ptr<AnimationWave> > waves_;
  GLboolean animateNormal_;

  virtual void set_data(void *data, GLuint offset);
};

///////

/**
 * Base class for animation waves.
 */
class AnimationWave : public EventObject
{
public:
  AnimationWave();

  /**
   * time difference in seconds
   */
  GLboolean timestep(GLdouble dt);

  /**
   * height of wave.
   */
  void set_amplitude(GLdouble amplitude);
  /**
   * height of wave.
   */
  GLdouble amplitude() const;

  /**
   * width of a single wave.
   */
  void set_width(GLdouble width);
  /**
   * width of a single wave.
   */
  GLdouble width() const;

  /**
   * velocity in units per second.
   */
  void set_velocity(GLdouble velocity);
  /**
   * velocity in units per second.
   */
  GLdouble velocity() const;

  /**
   * lifetime of this wave.
   */
  void set_lifetime(GLdouble lifetime);
  /**
   * lifetime of this wave.
   */
  GLdouble lifetime() const;

  /**
   * the vertices are pushed along their normal.
   * This is a factor applied to the operation.
   */
  void set_normalInfluence(GLdouble normalInfluence);
  /**
   * the vertices are pushed along their normal.
   * This is a factor applied to the operation.
   */
  GLdouble normalInfluence() const;

  /**
   * Update position.
   */
  virtual Vec3f calculateDisplacement(
      const Vec3f &v, const Vec3f &n) = 0;
  /**
   * Update normal.
   */
  virtual Vec3f calculateNormal(
      const Vec3f &v, const Vec3f &n) = 0;

  // override
  virtual void set_snapshot(vector<VecXf> &verts) { }

protected:
  GLdouble amplitude_;
  GLdouble width_;
  GLdouble offset_;
  GLdouble velocity_;
  GLdouble lifetime_;
  GLdouble lastX_;
  GLdouble waveFactor_;
  GLdouble normalInfluence_;

  float calculateWaveHeight(
      const Vec3f &v,
      GLdouble wavePosition);
  Vec3f calculateWaveNormal(
      const Mat4f &rot,
      const Vec3f &displacementDirection,
      const Vec3f &waveDirection);
};

/**
 * Emits directional waves along direction.
 */
class DirectionalAnimationWave : public AnimationWave
{
public:
  DirectionalAnimationWave();

  /**
   * the wave direction.
   */
  void set_direction(const Vec3f &direction);
  /**
   * the wave direction.
   */
  const Vec3f& direction() const;

  // override
  virtual Vec3f calculateDisplacement(const Vec3f &v, const Vec3f &n);
  virtual Vec3f calculateNormal(const Vec3f &v, const Vec3f &n);
  virtual void set_snapshot(vector<VecXf> &verts);
protected:
  Vec3f direction_;
  Vec3f displacementDirection_;
  Vec3f displacementV_;
  Mat4f rot_;

  void updateDisplacementDirection();
};

/**
 * Emits waves radial at position.
 */
class RadialAnimationWave : public AnimationWave {
public:
  RadialAnimationWave();

  /**
   * wave source position.
   */
  void set_position(const Vec3f &position);
  /**
   * wave source position.
   */
  const Vec3f& position() const;

  // override
  virtual Vec3f calculateDisplacement(const Vec3f &v, const Vec3f &n);
  virtual Vec3f calculateNormal(const Vec3f &v, const Vec3f &n);
protected:
  Vec3f position_;
};

#endif /* VBO_WAVE_ANIMATION_H_ */
