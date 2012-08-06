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

class VBOWaveAnimation : public VBOAnimation
{
public:
  VBOWaveAnimation(GLuint vbo, AttributeState &p, bool animateNormal=true);

  void addWave(ref_ptr<AnimationWave> wave);
  void removeWave(AnimationWave *wave);
  virtual void set_data(BufferData *data);

protected:
  list< ref_ptr<AnimationWave> > waves_;
  bool animateNormal_;

  // override
  virtual void doAnimate(const double &dt);
};

///////

class AnimationWave : public EventObject {
public:
  AnimationWave();

  /**
   * Set height of wave.
   */
  void set_amplitude(float amplitude);
  float amplitude() const;

  /**
   * Set the width of a single wave.
   */
  void set_width(float width);
  float width() const;

  /**
   * Set the velocity in units per second.
   */
  void set_velocity(float velocity);
  float velocity() const;

  void set_normalInfluence(float normalInfluence);
  float normalInfluence() const;

  void set_lifetime(float lifetime);
  float lifetime() const;

  virtual void set_snapshot(vector<VecXf> &verts) { }

  /**
   * @param dt difference in time in seconds
   */
  bool timestep(float dt);

  virtual Vec3f calculateDisplacement(const Vec3f &v, const Vec3f &n) = 0;
  virtual Vec3f calculateNormal(const Vec3f &v, const Vec3f &n) = 0;

protected:
  float amplitude_;
  float width_;
  float offset_;
  float velocity_;
  float lifetime_;
  float lastX_;
  float waveFactor_;
  float normalInfluence_;

  float calculateWaveHeight(const Vec3f &v, float wavePosition);
  Vec3f calculateWaveNormal(
      const Mat4f &rot,
      const Vec3f &displacementDirection,
      const Vec3f &waveDirection);
};

/**
 * Emits directional waves along direction.
 */
class DirectionalAnimationWave : public AnimationWave {
public:
  DirectionalAnimationWave();
  virtual Vec3f calculateDisplacement(const Vec3f &v, const Vec3f &n);
  virtual Vec3f calculateNormal(const Vec3f &v, const Vec3f &n);

  virtual void set_snapshot(vector<VecXf> &verts);

  void set_direction(const Vec3f &direction);
  const Vec3f& direction() const;
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
  virtual Vec3f calculateDisplacement(const Vec3f &v, const Vec3f &n);
  virtual Vec3f calculateNormal(const Vec3f &v, const Vec3f &n);

  void set_position(const Vec3f &position);
  const Vec3f& position() const;
protected:
  Vec3f position_;
};

#endif /* VBO_WAVE_ANIMATION_H_ */
